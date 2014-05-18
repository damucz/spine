#include <IwGx.h>
#include <s3ePointer.h>
#include <stdio.h>
#include <spine/spine.h>
#include <spine/extension.h>

void _spAtlasPage_createTexture (spAtlasPage* self, const char* path) {
  CIwTexture *texture = new CIwTexture();
  texture->LoadFromFile(path);
  texture->SetClamping(true);
  texture->SetMipMapping(false);
  texture->SetFiltering(true);
  texture->Upload();

  self->rendererObject = texture;
  self->width = texture->GetWidth();
  self->height = texture->GetHeight();
}

void _spAtlasPage_disposeTexture (spAtlasPage* self) {
  if (self->rendererObject)
    delete static_cast<CIwTexture*>(self->rendererObject);
}

char* _spUtil_readFile (const char* path, int* length) {
  s3eFile *f = s3eFileOpen(path, "rb");
  if (f)
  {
    *length = s3eFileGetSize(f);
    char *buffer = (char *)malloc(*length);
    *length = s3eFileRead(buffer, 1, *length, f);
    return buffer;
  }

  return  NULL;
}

spAtlas* atlas = NULL;
spSkeletonData* skeletonData = NULL;
spSkeleton* skeleton = NULL;
spAnimationState* state = NULL;
spAnimationStateData* stateData = NULL;
spSkeletonBounds* bounds = NULL;
spSlot* headSlot = NULL;        // spineboy's head
uint64 lastFrameTime = 0;

void SpineBoy()
{
    // Load atlas, skeleton, and animations.
    atlas = spAtlas_createFromFile("spineboy.atlas", 0);
    IwTrace(SPINE, ("First region name: %s, x: %d, y: %d\n", atlas->regions->name, atlas->regions->x, atlas->regions->y));
    IwTrace(SPINE, ("First page name: %s, size: %d, %d\n", atlas->pages->name, atlas->pages->width, atlas->pages->height));

    spSkeletonJson* json = spSkeletonJson_create(atlas);
    json->scale = 0.4f;
    skeletonData = spSkeletonJson_readSkeletonDataFile(json, "spineboy.json");
    if (!skeletonData) IwAssertMsg(SPINE, false, ("Error: %s\n", json->error));
    spSkeletonJson_dispose(json);
    IwTrace(SPINE, ("Default skin name: %s\n", skeletonData->defaultSkin->name));

    bounds = spSkeletonBounds_create();

    // Configure mixing.
    stateData = spAnimationStateData_create(skeletonData);
    spAnimationStateData_setMixByName(stateData, "walk", "jump", 0.2f);
    spAnimationStateData_setMixByName(stateData, "jump", "run", 0.2f);

    skeleton = spSkeleton_create(skeletonData);
    state = spAnimationState_create(stateData);

    skeleton->flipX = false;
    skeleton->flipY = false;
    spSkeleton_setToSetupPose(skeleton);

    skeleton->x = 150;
    skeleton->y = 400;
    spSkeleton_updateWorldTransform(skeleton);

    headSlot = spSkeleton_findSlot(skeleton, "head");

    spAnimationState_setAnimationByName(state, 0, "walk", true);
    spAnimationState_addAnimationByName(state, 0, "jump", false, 3);
    spAnimationState_addAnimationByName(state, 0, "run", true, 0);
}

void Goblins () {
    // Load atlas, skeleton, and animations.
    atlas = spAtlas_createFromFile("goblins-ffd.atlas", 0);
    IwTrace(SPINE, ("First region name: %s, x: %d, y: %d\n", atlas->regions->name, atlas->regions->x, atlas->regions->y));
    IwTrace(SPINE, ("First page name: %s, size: %d, %d\n", atlas->pages->name, atlas->pages->width, atlas->pages->height));

    spSkeletonJson* json = spSkeletonJson_create(atlas);
    json->scale = 1.0f;
    skeletonData = spSkeletonJson_readSkeletonDataFile(json, "goblins-ffd.json");
    if (!skeletonData) IwAssertMsg(SPINE, false, ("Error: %s\n", json->error));
    spAnimation* walkAnimation = spSkeletonData_findAnimation(skeletonData, "walk");
    spSkeletonJson_dispose(json);

    skeleton = spSkeleton_create(skeletonData);
    state = spAnimationState_create(NULL);

    skeleton->flipX = false;
    skeleton->flipY = false;
    spSkeleton_setSkinByName(skeleton, "goblin");
    spSkeleton_setSlotsToSetupPose(skeleton);
    //Skeleton_setAttachment(skeleton, "left hand item", "dagger");

    skeleton->x = 150;
    skeleton->y = 400;
    spSkeleton_updateWorldTransform(skeleton);

    spAnimationState_setAnimation(state, 0, walkAnimation, true);
}

void init() {
  IwGxInit();

  spBone_setYDown(true);

  //SpineBoy();
  Goblins();

  lastFrameTime = s3eTimerGetMs();
}

void release() {
  if (bounds) spSkeletonBounds_dispose(bounds);
  if (stateData) spAnimationStateData_dispose(stateData);
  spAnimationState_dispose(state);
  spSkeleton_dispose(skeleton);
  spSkeletonData_dispose(skeletonData);
  spAtlas_dispose(atlas);

  IwGxTerminate();
}

bool update() {
  float dt = (float)(s3eTimerGetMs() - lastFrameTime) / 1000.f;
  lastFrameTime = s3eTimerGetMs();

  if (bounds)
  {
      spSkeletonBounds_update(bounds, skeleton, true);

      if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_PRESSED)
      {

      }

      int pointerx = s3ePointerGetX();
      int pointery = s3ePointerGetY();

      IwTrace(SPINE, ("pointer: %d, %d", pointerx, pointery));
      if (spSkeletonBounds_containsPoint(bounds, (float)pointerx, (float)pointery))
      {
          IwAssert(SPINE, headSlot);
          headSlot->g = 0;
          headSlot->b = 0;
      }
      else
      {
          headSlot->g = 1.0f;
          headSlot->b = 1.0f;
      }
  }

  spSkeleton_update(skeleton, dt);
  spAnimationState_update(state, dt/* * timeScale*/);
  spAnimationState_apply(state, skeleton);
  spSkeleton_updateWorldTransform(skeleton);

  return true;
}

void draw() {
  IwGxSetColClear(0, 0, 0, 0);
  // Clear the screen
  IwGxClear(IW_GX_COLOUR_BUFFER_F | IW_GX_DEPTH_BUFFER_F);
  // Draw screen space in the back so 3D sprites are visible
  IwGxSetScreenSpaceSlot(-1);

  CIwTexture *lastTexture = NULL;
  CIwMaterial::AlphaMode lastBlend = CIwMaterial::ALPHA_NONE;

  for (int i = 0; i < skeleton->slotCount; ++i)
  {
    spSlot* slot = skeleton->slots[i];
    spAttachment* attachment = slot->attachment;
    if (!attachment) continue;

    CIwTexture *texture = NULL;
    CIwMaterial::AlphaMode blend = slot->data->additiveBlending ? CIwMaterial::ALPHA_ADD : CIwMaterial::ALPHA_BLEND;
    int num_vertices = 0;
    IwGxPrimType primType = IW_GX_QUAD_LIST;

    CIwFVec2 *uvs = NULL;
    CIwSVec2 *verts = NULL;
    CIwColour *colors = NULL;

    if (attachment->type == SP_ATTACHMENT_REGION)
    {
        num_vertices = 4;
        uvs = IW_GX_ALLOC(CIwFVec2, num_vertices);
        verts = IW_GX_ALLOC(CIwSVec2, num_vertices);
        colors = IW_GX_ALLOC(CIwColour, num_vertices);

        float worldVertices[8];
        spRegionAttachment* regionAttachment = (spRegionAttachment*)attachment;
        texture = static_cast<CIwTexture*>(((spAtlasRegion*)regionAttachment->rendererObject)->page->rendererObject);
        spRegionAttachment_computeWorldVertices(regionAttachment, slot->skeleton->x, slot->skeleton->y, slot->bone, worldVertices);

        unsigned char r = (unsigned char)(skeleton->r * slot->r * 255);
        unsigned char g = (unsigned char)(skeleton->g * slot->g * 255);
        unsigned char b = (unsigned char)(skeleton->b * slot->b * 255);
        unsigned char a = (unsigned char)(skeleton->a * slot->a * 255);

        for (int i = 0; i < num_vertices; ++i)
        {
            int xindex = i << 1;
            int yindex = xindex + 1;
            uvs[i].x = regionAttachment->uvs[xindex];
            uvs[i].y = regionAttachment->uvs[yindex];
            verts[i].x = (int)(worldVertices[xindex] * 8.f);
            verts[i].y = (int)(worldVertices[yindex] * 8.f);
            colors[i].Set(r, g, b, a);
        }
    }
    else if (attachment->type == SP_ATTACHMENT_MESH)
    {
        spMeshAttachment *mesh = (spMeshAttachment*)attachment;

        primType = IW_GX_TRI_LIST;
        num_vertices = mesh->trianglesCount;
        uvs = IW_GX_ALLOC(CIwFVec2, num_vertices);
        verts = IW_GX_ALLOC(CIwSVec2, num_vertices);
        colors = IW_GX_ALLOC(CIwColour, num_vertices);
        
        float *worldVertices = new float[mesh->verticesCount];
        texture = static_cast<CIwTexture*>(((spAtlasRegion*)mesh->rendererObject)->page->rendererObject);

        spMeshAttachment_computeWorldVertices(mesh, slot->skeleton->x, slot->skeleton->y, slot, worldVertices);

        unsigned char r = (unsigned char)(skeleton->r * slot->r * 255);
        unsigned char g = (unsigned char)(skeleton->g * slot->g * 255);
        unsigned char b = (unsigned char)(skeleton->b * slot->b * 255);
        unsigned char a = (unsigned char)(skeleton->a * slot->a * 255);

        for (int i = 0; i < mesh->trianglesCount; ++i)
        {
            int index = mesh->triangles[i] << 1;
            verts[i].x = (int)(worldVertices[index] * 8.f);
            verts[i].y = (int)(worldVertices[index+1] * 8.f);
            uvs[i].x = mesh->uvs[index];
            uvs[i].y = mesh->uvs[index+1];
            colors[i].Set(r, g, b, a);
        }

        delete[] worldVertices;
    }
    else if (attachment->type == SP_ATTACHMENT_SKINNED_MESH)
    {
        spSkinnedMeshAttachment* mesh = (spSkinnedMeshAttachment*)attachment;
        primType = IW_GX_TRI_LIST;
        num_vertices = mesh->trianglesCount;
        uvs = IW_GX_ALLOC(CIwFVec2, num_vertices);
        verts = IW_GX_ALLOC(CIwSVec2, num_vertices);
        colors = IW_GX_ALLOC(CIwColour, num_vertices);

        float *worldVertices = new float[mesh->uvsCount];
        texture = static_cast<CIwTexture*>(((spAtlasRegion*)mesh->rendererObject)->page->rendererObject);

        spSkinnedMeshAttachment_computeWorldVertices(mesh, slot->skeleton->x, slot->skeleton->y, slot, worldVertices);

        unsigned char r = (unsigned char)(skeleton->r * slot->r * 255);
        unsigned char g = (unsigned char)(skeleton->g * slot->g * 255);
        unsigned char b = (unsigned char)(skeleton->b * slot->b * 255);
        unsigned char a = (unsigned char)(skeleton->a * slot->a * 255);

        for (int i = 0; i < mesh->trianglesCount; ++i)
        {
            int index = mesh->triangles[i] << 1;
            verts[i].x = (int)(worldVertices[index] * 8.f);
            verts[i].y = (int)(worldVertices[index+1] * 8.f);
            uvs[i].x = mesh->uvs[index];
            uvs[i].y = mesh->uvs[index+1];
            colors[i].Set(r, g, b, a);
        }

        delete[] worldVertices;
    }

    IwAssert(SPINE, texture);
    if (!texture) return;

    // primitive caching of the material texture
    if (lastTexture != texture || lastBlend != blend)
    {
        CIwMaterial* mat = IW_GX_ALLOC_MATERIAL();
        mat->SetTexture(texture);
        mat->SetDepthWriteMode(CIwMaterial::DEPTH_WRITE_DISABLED);
        mat->SetAlphaMode(blend);
        mat->SetCullMode(CIwMaterial::CULL_NONE);
        IwGxSetMaterial(mat);		// can be cached to improve performance
        lastTexture = texture;
        lastBlend = blend;
    }

    IwGxSetUVStream(uvs);
    IwGxSetVertStreamScreenSpaceSubPixel(verts, num_vertices);
    IwGxSetColStream(colors, num_vertices);
    IwGxSetNormStream(NULL);
    IwGxDrawPrims(primType, NULL, num_vertices);
  }


  //Iw2DFinishDrawing();

  // Flush IwGx
  IwGxFlush();

  // Display the rendered frame
  IwGxSwapBuffers();

  // Show the surface
  //	Iw2DSurfaceShow();
}

S3E_MAIN_DECL void IwMain() {

  init();

  // Main Game Loop
  while (!s3eDeviceCheckQuitRequest())
  {
    // Yield to the operating system
    s3eDeviceYield(0);

    // Prevent back-light to off state
    s3eDeviceBacklightOn();

    // Update the game
    if (!update())
      break;

    // Draw the scene
    draw();
  }

  release();

}
