// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 Petrus Silva Costa
//
// EN: GENERATED FILE -- do not hand-edit. See gl_loader.h / tools/gen_glloader.py.
//
//     glx_gl_load() resolution order per symbol, matching the L1.14-GLLOADER task
//     brief: (1) glXGetProcAddressARB/glXGetProcAddress, dlsym'd out of the libGL.so.1
//     handle -- covers GLX-backed desktop contexts (GLFW's default on X11/Mesa,
//     the CI/dev target here); (2) eglGetProcAddress, dlsym'd out of a libEGL.so.1
//     handle if present -- covers EGL-backed contexts (Wayland, some embedded
//     hosts); (3) a direct dlsym() against the libGL handle -- covers core entry
//     points that are ordinary exported symbols on most GL/Mesa builds and do not
//     strictly require a GetProcAddress indirection. The first non-NULL result
//     wins.
// PT: GENERATED FILE -- não editar à mão. Ver gl_loader.h / tools/gen_glloader.py.
//
//     Ordem de resolução por símbolo em glx_gl_load(), conforme o brief da tarefa
//     L1.14-GLLOADER: (1) glXGetProcAddressARB/glXGetProcAddress, via dlsym no
//     handle de libGL.so.1 -- cobre contextos desktop com backing GLX (padrão do
//     GLFW em X11/Mesa, o alvo de CI/dev aqui); (2) eglGetProcAddress, via dlsym
//     num handle de libEGL.so.1 se presente -- cobre contextos com backing EGL
//     (Wayland, alguns hosts embarcados); (3) dlsym() direto no handle da libGL --
//     cobre entry points core que são símbolos exportados comuns na maioria das
//     builds GL/Mesa e não exigem estritamente indireção via GetProcAddress. O
//     primeiro resultado não-NULL vence.
#include "gl_loader.h"

#include <dlfcn.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
PFNGLATTACHSHADERPROC glAttachShader = NULL;
PFNGLBEGINCONDITIONALRENDERPROC glBeginConditionalRender = NULL;
PFNGLBEGINQUERYPROC glBeginQuery = NULL;
PFNGLBEGINTRANSFORMFEEDBACKPROC glBeginTransformFeedback = NULL;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation = NULL;
PFNGLBINDBUFFERPROC glBindBuffer = NULL;
PFNGLBINDBUFFERBASEPROC glBindBufferBase = NULL;
PFNGLBINDBUFFERRANGEPROC glBindBufferRange = NULL;
PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation = NULL;
PFNGLBINDFRAGDATALOCATIONINDEXEDPROC glBindFragDataLocationIndexed = NULL;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer = NULL;
PFNGLBINDSAMPLERPROC glBindSampler = NULL;
PFNGLBINDTEXTUREPROC glBindTexture = NULL;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
PFNGLBLENDCOLORPROC glBlendColor = NULL;
PFNGLBLENDEQUATIONPROC glBlendEquation = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC glBlendEquationSeparate = NULL;
PFNGLBLENDFUNCPROC glBlendFunc = NULL;
PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate = NULL;
PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer = NULL;
PFNGLBUFFERDATAPROC glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glBufferSubData = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = NULL;
PFNGLCLAMPCOLORPROC glClampColor = NULL;
PFNGLCLEARPROC glClear = NULL;
PFNGLCLEARBUFFERFIPROC glClearBufferfi = NULL;
PFNGLCLEARBUFFERFVPROC glClearBufferfv = NULL;
PFNGLCLEARBUFFERIVPROC glClearBufferiv = NULL;
PFNGLCLEARBUFFERUIVPROC glClearBufferuiv = NULL;
PFNGLCLEARCOLORPROC glClearColor = NULL;
PFNGLCLEARDEPTHPROC glClearDepth = NULL;
PFNGLCLEARSTENCILPROC glClearStencil = NULL;
PFNGLCLIENTWAITSYNCPROC glClientWaitSync = NULL;
PFNGLCOLORMASKPROC glColorMask = NULL;
PFNGLCOLORMASKIPROC glColorMaski = NULL;
PFNGLCOMPILESHADERPROC glCompileShader = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DPROC glCompressedTexImage1D = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DPROC glCompressedTexImage3D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glCompressedTexSubImage1D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glCompressedTexSubImage3D = NULL;
PFNGLCOPYBUFFERSUBDATAPROC glCopyBufferSubData = NULL;
PFNGLCOPYTEXIMAGE1DPROC glCopyTexImage1D = NULL;
PFNGLCOPYTEXIMAGE2DPROC glCopyTexImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE1DPROC glCopyTexSubImage1D = NULL;
PFNGLCOPYTEXSUBIMAGE2DPROC glCopyTexSubImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE3DPROC glCopyTexSubImage3D = NULL;
PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
PFNGLCREATESHADERPROC glCreateShader = NULL;
PFNGLCULLFACEPROC glCullFace = NULL;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = NULL;
PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
PFNGLDELETEQUERIESPROC glDeleteQueries = NULL;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers = NULL;
PFNGLDELETESAMPLERSPROC glDeleteSamplers = NULL;
PFNGLDELETESHADERPROC glDeleteShader = NULL;
PFNGLDELETESYNCPROC glDeleteSync = NULL;
PFNGLDELETETEXTURESPROC glDeleteTextures = NULL;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;
PFNGLDEPTHFUNCPROC glDepthFunc = NULL;
PFNGLDEPTHMASKPROC glDepthMask = NULL;
PFNGLDEPTHRANGEPROC glDepthRange = NULL;
PFNGLDETACHSHADERPROC glDetachShader = NULL;
PFNGLDISABLEPROC glDisable = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray = NULL;
PFNGLDISABLEIPROC glDisablei = NULL;
PFNGLDRAWARRAYSPROC glDrawArrays = NULL;
PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced = NULL;
PFNGLDRAWBUFFERPROC glDrawBuffer = NULL;
PFNGLDRAWBUFFERSPROC glDrawBuffers = NULL;
PFNGLDRAWELEMENTSPROC glDrawElements = NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC glDrawElementsBaseVertex = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glDrawElementsInstancedBaseVertex = NULL;
PFNGLDRAWRANGEELEMENTSPROC glDrawRangeElements = NULL;
PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC glDrawRangeElementsBaseVertex = NULL;
PFNGLENABLEPROC glEnable = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;
PFNGLENABLEIPROC glEnablei = NULL;
PFNGLENDCONDITIONALRENDERPROC glEndConditionalRender = NULL;
PFNGLENDQUERYPROC glEndQuery = NULL;
PFNGLENDTRANSFORMFEEDBACKPROC glEndTransformFeedback = NULL;
PFNGLFENCESYNCPROC glFenceSync = NULL;
PFNGLFINISHPROC glFinish = NULL;
PFNGLFLUSHPROC glFlush = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC glFlushMappedBufferRange = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture = NULL;
PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D = NULL;
PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer = NULL;
PFNGLFRONTFACEPROC glFrontFace = NULL;
PFNGLGENBUFFERSPROC glGenBuffers = NULL;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = NULL;
PFNGLGENQUERIESPROC glGenQueries = NULL;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers = NULL;
PFNGLGENSAMPLERSPROC glGenSamplers = NULL;
PFNGLGENTEXTURESPROC glGenTextures = NULL;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
PFNGLGENERATEMIPMAPPROC glGenerateMipmap = NULL;
PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib = NULL;
PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform = NULL;
PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC glGetActiveUniformBlockName = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glGetActiveUniformBlockiv = NULL;
PFNGLGETACTIVEUNIFORMNAMEPROC glGetActiveUniformName = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC glGetActiveUniformsiv = NULL;
PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders = NULL;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = NULL;
PFNGLGETBOOLEANI_VPROC glGetBooleani_v = NULL;
PFNGLGETBOOLEANVPROC glGetBooleanv = NULL;
PFNGLGETBUFFERPARAMETERI64VPROC glGetBufferParameteri64v = NULL;
PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv = NULL;
PFNGLGETBUFFERPOINTERVPROC glGetBufferPointerv = NULL;
PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEPROC glGetCompressedTexImage = NULL;
PFNGLGETDOUBLEVPROC glGetDoublev = NULL;
PFNGLGETERRORPROC glGetError = NULL;
PFNGLGETFLOATVPROC glGetFloatv = NULL;
PFNGLGETFRAGDATAINDEXPROC glGetFragDataIndex = NULL;
PFNGLGETFRAGDATALOCATIONPROC glGetFragDataLocation = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGETINTEGER64I_VPROC glGetInteger64i_v = NULL;
PFNGLGETINTEGER64VPROC glGetInteger64v = NULL;
PFNGLGETINTEGERI_VPROC glGetIntegeri_v = NULL;
PFNGLGETINTEGERVPROC glGetIntegerv = NULL;
PFNGLGETMULTISAMPLEFVPROC glGetMultisamplefv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
PFNGLGETQUERYOBJECTI64VPROC glGetQueryObjecti64v = NULL;
PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v = NULL;
PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv = NULL;
PFNGLGETQUERYIVPROC glGetQueryiv = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv = NULL;
PFNGLGETSAMPLERPARAMETERIIVPROC glGetSamplerParameterIiv = NULL;
PFNGLGETSAMPLERPARAMETERIUIVPROC glGetSamplerParameterIuiv = NULL;
PFNGLGETSAMPLERPARAMETERFVPROC glGetSamplerParameterfv = NULL;
PFNGLGETSAMPLERPARAMETERIVPROC glGetSamplerParameteriv = NULL;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
PFNGLGETSHADERSOURCEPROC glGetShaderSource = NULL;
PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
PFNGLGETSTRINGPROC glGetString = NULL;
PFNGLGETSTRINGIPROC glGetStringi = NULL;
PFNGLGETSYNCIVPROC glGetSynciv = NULL;
PFNGLGETTEXIMAGEPROC glGetTexImage = NULL;
PFNGLGETTEXLEVELPARAMETERFVPROC glGetTexLevelParameterfv = NULL;
PFNGLGETTEXLEVELPARAMETERIVPROC glGetTexLevelParameteriv = NULL;
PFNGLGETTEXPARAMETERIIVPROC glGetTexParameterIiv = NULL;
PFNGLGETTEXPARAMETERIUIVPROC glGetTexParameterIuiv = NULL;
PFNGLGETTEXPARAMETERFVPROC glGetTexParameterfv = NULL;
PFNGLGETTEXPARAMETERIVPROC glGetTexParameteriv = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC glGetTransformFeedbackVarying = NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex = NULL;
PFNGLGETUNIFORMINDICESPROC glGetUniformIndices = NULL;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
PFNGLGETUNIFORMFVPROC glGetUniformfv = NULL;
PFNGLGETUNIFORMIVPROC glGetUniformiv = NULL;
PFNGLGETUNIFORMUIVPROC glGetUniformuiv = NULL;
PFNGLGETVERTEXATTRIBIIVPROC glGetVertexAttribIiv = NULL;
PFNGLGETVERTEXATTRIBIUIVPROC glGetVertexAttribIuiv = NULL;
PFNGLGETVERTEXATTRIBPOINTERVPROC glGetVertexAttribPointerv = NULL;
PFNGLGETVERTEXATTRIBDVPROC glGetVertexAttribdv = NULL;
PFNGLGETVERTEXATTRIBFVPROC glGetVertexAttribfv = NULL;
PFNGLGETVERTEXATTRIBIVPROC glGetVertexAttribiv = NULL;
PFNGLHINTPROC glHint = NULL;
PFNGLISBUFFERPROC glIsBuffer = NULL;
PFNGLISENABLEDPROC glIsEnabled = NULL;
PFNGLISENABLEDIPROC glIsEnabledi = NULL;
PFNGLISFRAMEBUFFERPROC glIsFramebuffer = NULL;
PFNGLISPROGRAMPROC glIsProgram = NULL;
PFNGLISQUERYPROC glIsQuery = NULL;
PFNGLISRENDERBUFFERPROC glIsRenderbuffer = NULL;
PFNGLISSAMPLERPROC glIsSampler = NULL;
PFNGLISSHADERPROC glIsShader = NULL;
PFNGLISSYNCPROC glIsSync = NULL;
PFNGLISTEXTUREPROC glIsTexture = NULL;
PFNGLISVERTEXARRAYPROC glIsVertexArray = NULL;
PFNGLLINEWIDTHPROC glLineWidth = NULL;
PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
PFNGLLOGICOPPROC glLogicOp = NULL;
PFNGLMAPBUFFERPROC glMapBuffer = NULL;
PFNGLMAPBUFFERRANGEPROC glMapBufferRange = NULL;
PFNGLMULTIDRAWARRAYSPROC glMultiDrawArrays = NULL;
PFNGLMULTIDRAWELEMENTSPROC glMultiDrawElements = NULL;
PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC glMultiDrawElementsBaseVertex = NULL;
PFNGLPIXELSTOREFPROC glPixelStoref = NULL;
PFNGLPIXELSTOREIPROC glPixelStorei = NULL;
PFNGLPOINTPARAMETERFPROC glPointParameterf = NULL;
PFNGLPOINTPARAMETERFVPROC glPointParameterfv = NULL;
PFNGLPOINTPARAMETERIPROC glPointParameteri = NULL;
PFNGLPOINTPARAMETERIVPROC glPointParameteriv = NULL;
PFNGLPOINTSIZEPROC glPointSize = NULL;
PFNGLPOLYGONMODEPROC glPolygonMode = NULL;
PFNGLPOLYGONOFFSETPROC glPolygonOffset = NULL;
PFNGLPRIMITIVERESTARTINDEXPROC glPrimitiveRestartIndex = NULL;
PFNGLPROVOKINGVERTEXPROC glProvokingVertex = NULL;
PFNGLQUERYCOUNTERPROC glQueryCounter = NULL;
PFNGLREADBUFFERPROC glReadBuffer = NULL;
PFNGLREADPIXELSPROC glReadPixels = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample = NULL;
PFNGLSAMPLECOVERAGEPROC glSampleCoverage = NULL;
PFNGLSAMPLEMASKIPROC glSampleMaski = NULL;
PFNGLSAMPLERPARAMETERIIVPROC glSamplerParameterIiv = NULL;
PFNGLSAMPLERPARAMETERIUIVPROC glSamplerParameterIuiv = NULL;
PFNGLSAMPLERPARAMETERFPROC glSamplerParameterf = NULL;
PFNGLSAMPLERPARAMETERFVPROC glSamplerParameterfv = NULL;
PFNGLSAMPLERPARAMETERIPROC glSamplerParameteri = NULL;
PFNGLSAMPLERPARAMETERIVPROC glSamplerParameteriv = NULL;
PFNGLSCISSORPROC glScissor = NULL;
PFNGLSHADERSOURCEPROC glShaderSource = NULL;
PFNGLSTENCILFUNCPROC glStencilFunc = NULL;
PFNGLSTENCILFUNCSEPARATEPROC glStencilFuncSeparate = NULL;
PFNGLSTENCILMASKPROC glStencilMask = NULL;
PFNGLSTENCILMASKSEPARATEPROC glStencilMaskSeparate = NULL;
PFNGLSTENCILOPPROC glStencilOp = NULL;
PFNGLSTENCILOPSEPARATEPROC glStencilOpSeparate = NULL;
PFNGLTEXBUFFERPROC glTexBuffer = NULL;
PFNGLTEXIMAGE1DPROC glTexImage1D = NULL;
PFNGLTEXIMAGE2DPROC glTexImage2D = NULL;
PFNGLTEXIMAGE2DMULTISAMPLEPROC glTexImage2DMultisample = NULL;
PFNGLTEXIMAGE3DPROC glTexImage3D = NULL;
PFNGLTEXIMAGE3DMULTISAMPLEPROC glTexImage3DMultisample = NULL;
PFNGLTEXPARAMETERIIVPROC glTexParameterIiv = NULL;
PFNGLTEXPARAMETERIUIVPROC glTexParameterIuiv = NULL;
PFNGLTEXPARAMETERFPROC glTexParameterf = NULL;
PFNGLTEXPARAMETERFVPROC glTexParameterfv = NULL;
PFNGLTEXPARAMETERIPROC glTexParameteri = NULL;
PFNGLTEXPARAMETERIVPROC glTexParameteriv = NULL;
PFNGLTEXSUBIMAGE1DPROC glTexSubImage1D = NULL;
PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D = NULL;
PFNGLTEXSUBIMAGE3DPROC glTexSubImage3D = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC glTransformFeedbackVaryings = NULL;
PFNGLUNIFORM1FPROC glUniform1f = NULL;
PFNGLUNIFORM1FVPROC glUniform1fv = NULL;
PFNGLUNIFORM1IPROC glUniform1i = NULL;
PFNGLUNIFORM1IVPROC glUniform1iv = NULL;
PFNGLUNIFORM1UIPROC glUniform1ui = NULL;
PFNGLUNIFORM1UIVPROC glUniform1uiv = NULL;
PFNGLUNIFORM2FPROC glUniform2f = NULL;
PFNGLUNIFORM2FVPROC glUniform2fv = NULL;
PFNGLUNIFORM2IPROC glUniform2i = NULL;
PFNGLUNIFORM2IVPROC glUniform2iv = NULL;
PFNGLUNIFORM2UIPROC glUniform2ui = NULL;
PFNGLUNIFORM2UIVPROC glUniform2uiv = NULL;
PFNGLUNIFORM3FPROC glUniform3f = NULL;
PFNGLUNIFORM3FVPROC glUniform3fv = NULL;
PFNGLUNIFORM3IPROC glUniform3i = NULL;
PFNGLUNIFORM3IVPROC glUniform3iv = NULL;
PFNGLUNIFORM3UIPROC glUniform3ui = NULL;
PFNGLUNIFORM3UIVPROC glUniform3uiv = NULL;
PFNGLUNIFORM4FPROC glUniform4f = NULL;
PFNGLUNIFORM4FVPROC glUniform4fv = NULL;
PFNGLUNIFORM4IPROC glUniform4i = NULL;
PFNGLUNIFORM4IVPROC glUniform4iv = NULL;
PFNGLUNIFORM4UIPROC glUniform4ui = NULL;
PFNGLUNIFORM4UIVPROC glUniform4uiv = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding = NULL;
PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv = NULL;
PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv = NULL;
PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv = NULL;
PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv = NULL;
PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv = NULL;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv = NULL;
PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv = NULL;
PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv = NULL;
PFNGLUNMAPBUFFERPROC glUnmapBuffer = NULL;
PFNGLUSEPROGRAMPROC glUseProgram = NULL;
PFNGLVALIDATEPROGRAMPROC glValidateProgram = NULL;
PFNGLVERTEXATTRIB1DPROC glVertexAttrib1d = NULL;
PFNGLVERTEXATTRIB1DVPROC glVertexAttrib1dv = NULL;
PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f = NULL;
PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv = NULL;
PFNGLVERTEXATTRIB1SPROC glVertexAttrib1s = NULL;
PFNGLVERTEXATTRIB1SVPROC glVertexAttrib1sv = NULL;
PFNGLVERTEXATTRIB2DPROC glVertexAttrib2d = NULL;
PFNGLVERTEXATTRIB2DVPROC glVertexAttrib2dv = NULL;
PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f = NULL;
PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv = NULL;
PFNGLVERTEXATTRIB2SPROC glVertexAttrib2s = NULL;
PFNGLVERTEXATTRIB2SVPROC glVertexAttrib2sv = NULL;
PFNGLVERTEXATTRIB3DPROC glVertexAttrib3d = NULL;
PFNGLVERTEXATTRIB3DVPROC glVertexAttrib3dv = NULL;
PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv = NULL;
PFNGLVERTEXATTRIB3SPROC glVertexAttrib3s = NULL;
PFNGLVERTEXATTRIB3SVPROC glVertexAttrib3sv = NULL;
PFNGLVERTEXATTRIB4NBVPROC glVertexAttrib4Nbv = NULL;
PFNGLVERTEXATTRIB4NIVPROC glVertexAttrib4Niv = NULL;
PFNGLVERTEXATTRIB4NSVPROC glVertexAttrib4Nsv = NULL;
PFNGLVERTEXATTRIB4NUBPROC glVertexAttrib4Nub = NULL;
PFNGLVERTEXATTRIB4NUBVPROC glVertexAttrib4Nubv = NULL;
PFNGLVERTEXATTRIB4NUIVPROC glVertexAttrib4Nuiv = NULL;
PFNGLVERTEXATTRIB4NUSVPROC glVertexAttrib4Nusv = NULL;
PFNGLVERTEXATTRIB4BVPROC glVertexAttrib4bv = NULL;
PFNGLVERTEXATTRIB4DPROC glVertexAttrib4d = NULL;
PFNGLVERTEXATTRIB4DVPROC glVertexAttrib4dv = NULL;
PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f = NULL;
PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv = NULL;
PFNGLVERTEXATTRIB4IVPROC glVertexAttrib4iv = NULL;
PFNGLVERTEXATTRIB4SPROC glVertexAttrib4s = NULL;
PFNGLVERTEXATTRIB4SVPROC glVertexAttrib4sv = NULL;
PFNGLVERTEXATTRIB4UBVPROC glVertexAttrib4ubv = NULL;
PFNGLVERTEXATTRIB4UIVPROC glVertexAttrib4uiv = NULL;
PFNGLVERTEXATTRIB4USVPROC glVertexAttrib4usv = NULL;
PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor = NULL;
PFNGLVERTEXATTRIBI1IPROC glVertexAttribI1i = NULL;
PFNGLVERTEXATTRIBI1IVPROC glVertexAttribI1iv = NULL;
PFNGLVERTEXATTRIBI1UIPROC glVertexAttribI1ui = NULL;
PFNGLVERTEXATTRIBI1UIVPROC glVertexAttribI1uiv = NULL;
PFNGLVERTEXATTRIBI2IPROC glVertexAttribI2i = NULL;
PFNGLVERTEXATTRIBI2IVPROC glVertexAttribI2iv = NULL;
PFNGLVERTEXATTRIBI2UIPROC glVertexAttribI2ui = NULL;
PFNGLVERTEXATTRIBI2UIVPROC glVertexAttribI2uiv = NULL;
PFNGLVERTEXATTRIBI3IPROC glVertexAttribI3i = NULL;
PFNGLVERTEXATTRIBI3IVPROC glVertexAttribI3iv = NULL;
PFNGLVERTEXATTRIBI3UIPROC glVertexAttribI3ui = NULL;
PFNGLVERTEXATTRIBI3UIVPROC glVertexAttribI3uiv = NULL;
PFNGLVERTEXATTRIBI4BVPROC glVertexAttribI4bv = NULL;
PFNGLVERTEXATTRIBI4IPROC glVertexAttribI4i = NULL;
PFNGLVERTEXATTRIBI4IVPROC glVertexAttribI4iv = NULL;
PFNGLVERTEXATTRIBI4SVPROC glVertexAttribI4sv = NULL;
PFNGLVERTEXATTRIBI4UBVPROC glVertexAttribI4ubv = NULL;
PFNGLVERTEXATTRIBI4UIPROC glVertexAttribI4ui = NULL;
PFNGLVERTEXATTRIBI4UIVPROC glVertexAttribI4uiv = NULL;
PFNGLVERTEXATTRIBI4USVPROC glVertexAttribI4usv = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer = NULL;
PFNGLVERTEXATTRIBP1UIPROC glVertexAttribP1ui = NULL;
PFNGLVERTEXATTRIBP1UIVPROC glVertexAttribP1uiv = NULL;
PFNGLVERTEXATTRIBP2UIPROC glVertexAttribP2ui = NULL;
PFNGLVERTEXATTRIBP2UIVPROC glVertexAttribP2uiv = NULL;
PFNGLVERTEXATTRIBP3UIPROC glVertexAttribP3ui = NULL;
PFNGLVERTEXATTRIBP3UIVPROC glVertexAttribP3uiv = NULL;
PFNGLVERTEXATTRIBP4UIPROC glVertexAttribP4ui = NULL;
PFNGLVERTEXATTRIBP4UIVPROC glVertexAttribP4uiv = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
PFNGLVIEWPORTPROC glViewport = NULL;
PFNGLWAITSYNCPROC glWaitSync = NULL;

typedef struct { const char* name; void** slot; } glintfx_gl_sym_t;

static const glintfx_gl_sym_t glintfx_gl_symbol_table[] = {
  { "glActiveTexture", (void**)&glActiveTexture },
  { "glAttachShader", (void**)&glAttachShader },
  { "glBeginConditionalRender", (void**)&glBeginConditionalRender },
  { "glBeginQuery", (void**)&glBeginQuery },
  { "glBeginTransformFeedback", (void**)&glBeginTransformFeedback },
  { "glBindAttribLocation", (void**)&glBindAttribLocation },
  { "glBindBuffer", (void**)&glBindBuffer },
  { "glBindBufferBase", (void**)&glBindBufferBase },
  { "glBindBufferRange", (void**)&glBindBufferRange },
  { "glBindFragDataLocation", (void**)&glBindFragDataLocation },
  { "glBindFragDataLocationIndexed", (void**)&glBindFragDataLocationIndexed },
  { "glBindFramebuffer", (void**)&glBindFramebuffer },
  { "glBindRenderbuffer", (void**)&glBindRenderbuffer },
  { "glBindSampler", (void**)&glBindSampler },
  { "glBindTexture", (void**)&glBindTexture },
  { "glBindVertexArray", (void**)&glBindVertexArray },
  { "glBlendColor", (void**)&glBlendColor },
  { "glBlendEquation", (void**)&glBlendEquation },
  { "glBlendEquationSeparate", (void**)&glBlendEquationSeparate },
  { "glBlendFunc", (void**)&glBlendFunc },
  { "glBlendFuncSeparate", (void**)&glBlendFuncSeparate },
  { "glBlitFramebuffer", (void**)&glBlitFramebuffer },
  { "glBufferData", (void**)&glBufferData },
  { "glBufferSubData", (void**)&glBufferSubData },
  { "glCheckFramebufferStatus", (void**)&glCheckFramebufferStatus },
  { "glClampColor", (void**)&glClampColor },
  { "glClear", (void**)&glClear },
  { "glClearBufferfi", (void**)&glClearBufferfi },
  { "glClearBufferfv", (void**)&glClearBufferfv },
  { "glClearBufferiv", (void**)&glClearBufferiv },
  { "glClearBufferuiv", (void**)&glClearBufferuiv },
  { "glClearColor", (void**)&glClearColor },
  { "glClearDepth", (void**)&glClearDepth },
  { "glClearStencil", (void**)&glClearStencil },
  { "glClientWaitSync", (void**)&glClientWaitSync },
  { "glColorMask", (void**)&glColorMask },
  { "glColorMaski", (void**)&glColorMaski },
  { "glCompileShader", (void**)&glCompileShader },
  { "glCompressedTexImage1D", (void**)&glCompressedTexImage1D },
  { "glCompressedTexImage2D", (void**)&glCompressedTexImage2D },
  { "glCompressedTexImage3D", (void**)&glCompressedTexImage3D },
  { "glCompressedTexSubImage1D", (void**)&glCompressedTexSubImage1D },
  { "glCompressedTexSubImage2D", (void**)&glCompressedTexSubImage2D },
  { "glCompressedTexSubImage3D", (void**)&glCompressedTexSubImage3D },
  { "glCopyBufferSubData", (void**)&glCopyBufferSubData },
  { "glCopyTexImage1D", (void**)&glCopyTexImage1D },
  { "glCopyTexImage2D", (void**)&glCopyTexImage2D },
  { "glCopyTexSubImage1D", (void**)&glCopyTexSubImage1D },
  { "glCopyTexSubImage2D", (void**)&glCopyTexSubImage2D },
  { "glCopyTexSubImage3D", (void**)&glCopyTexSubImage3D },
  { "glCreateProgram", (void**)&glCreateProgram },
  { "glCreateShader", (void**)&glCreateShader },
  { "glCullFace", (void**)&glCullFace },
  { "glDeleteBuffers", (void**)&glDeleteBuffers },
  { "glDeleteFramebuffers", (void**)&glDeleteFramebuffers },
  { "glDeleteProgram", (void**)&glDeleteProgram },
  { "glDeleteQueries", (void**)&glDeleteQueries },
  { "glDeleteRenderbuffers", (void**)&glDeleteRenderbuffers },
  { "glDeleteSamplers", (void**)&glDeleteSamplers },
  { "glDeleteShader", (void**)&glDeleteShader },
  { "glDeleteSync", (void**)&glDeleteSync },
  { "glDeleteTextures", (void**)&glDeleteTextures },
  { "glDeleteVertexArrays", (void**)&glDeleteVertexArrays },
  { "glDepthFunc", (void**)&glDepthFunc },
  { "glDepthMask", (void**)&glDepthMask },
  { "glDepthRange", (void**)&glDepthRange },
  { "glDetachShader", (void**)&glDetachShader },
  { "glDisable", (void**)&glDisable },
  { "glDisableVertexAttribArray", (void**)&glDisableVertexAttribArray },
  { "glDisablei", (void**)&glDisablei },
  { "glDrawArrays", (void**)&glDrawArrays },
  { "glDrawArraysInstanced", (void**)&glDrawArraysInstanced },
  { "glDrawBuffer", (void**)&glDrawBuffer },
  { "glDrawBuffers", (void**)&glDrawBuffers },
  { "glDrawElements", (void**)&glDrawElements },
  { "glDrawElementsBaseVertex", (void**)&glDrawElementsBaseVertex },
  { "glDrawElementsInstanced", (void**)&glDrawElementsInstanced },
  { "glDrawElementsInstancedBaseVertex", (void**)&glDrawElementsInstancedBaseVertex },
  { "glDrawRangeElements", (void**)&glDrawRangeElements },
  { "glDrawRangeElementsBaseVertex", (void**)&glDrawRangeElementsBaseVertex },
  { "glEnable", (void**)&glEnable },
  { "glEnableVertexAttribArray", (void**)&glEnableVertexAttribArray },
  { "glEnablei", (void**)&glEnablei },
  { "glEndConditionalRender", (void**)&glEndConditionalRender },
  { "glEndQuery", (void**)&glEndQuery },
  { "glEndTransformFeedback", (void**)&glEndTransformFeedback },
  { "glFenceSync", (void**)&glFenceSync },
  { "glFinish", (void**)&glFinish },
  { "glFlush", (void**)&glFlush },
  { "glFlushMappedBufferRange", (void**)&glFlushMappedBufferRange },
  { "glFramebufferRenderbuffer", (void**)&glFramebufferRenderbuffer },
  { "glFramebufferTexture", (void**)&glFramebufferTexture },
  { "glFramebufferTexture1D", (void**)&glFramebufferTexture1D },
  { "glFramebufferTexture2D", (void**)&glFramebufferTexture2D },
  { "glFramebufferTexture3D", (void**)&glFramebufferTexture3D },
  { "glFramebufferTextureLayer", (void**)&glFramebufferTextureLayer },
  { "glFrontFace", (void**)&glFrontFace },
  { "glGenBuffers", (void**)&glGenBuffers },
  { "glGenFramebuffers", (void**)&glGenFramebuffers },
  { "glGenQueries", (void**)&glGenQueries },
  { "glGenRenderbuffers", (void**)&glGenRenderbuffers },
  { "glGenSamplers", (void**)&glGenSamplers },
  { "glGenTextures", (void**)&glGenTextures },
  { "glGenVertexArrays", (void**)&glGenVertexArrays },
  { "glGenerateMipmap", (void**)&glGenerateMipmap },
  { "glGetActiveAttrib", (void**)&glGetActiveAttrib },
  { "glGetActiveUniform", (void**)&glGetActiveUniform },
  { "glGetActiveUniformBlockName", (void**)&glGetActiveUniformBlockName },
  { "glGetActiveUniformBlockiv", (void**)&glGetActiveUniformBlockiv },
  { "glGetActiveUniformName", (void**)&glGetActiveUniformName },
  { "glGetActiveUniformsiv", (void**)&glGetActiveUniformsiv },
  { "glGetAttachedShaders", (void**)&glGetAttachedShaders },
  { "glGetAttribLocation", (void**)&glGetAttribLocation },
  { "glGetBooleani_v", (void**)&glGetBooleani_v },
  { "glGetBooleanv", (void**)&glGetBooleanv },
  { "glGetBufferParameteri64v", (void**)&glGetBufferParameteri64v },
  { "glGetBufferParameteriv", (void**)&glGetBufferParameteriv },
  { "glGetBufferPointerv", (void**)&glGetBufferPointerv },
  { "glGetBufferSubData", (void**)&glGetBufferSubData },
  { "glGetCompressedTexImage", (void**)&glGetCompressedTexImage },
  { "glGetDoublev", (void**)&glGetDoublev },
  { "glGetError", (void**)&glGetError },
  { "glGetFloatv", (void**)&glGetFloatv },
  { "glGetFragDataIndex", (void**)&glGetFragDataIndex },
  { "glGetFragDataLocation", (void**)&glGetFragDataLocation },
  { "glGetFramebufferAttachmentParameteriv", (void**)&glGetFramebufferAttachmentParameteriv },
  { "glGetInteger64i_v", (void**)&glGetInteger64i_v },
  { "glGetInteger64v", (void**)&glGetInteger64v },
  { "glGetIntegeri_v", (void**)&glGetIntegeri_v },
  { "glGetIntegerv", (void**)&glGetIntegerv },
  { "glGetMultisamplefv", (void**)&glGetMultisamplefv },
  { "glGetProgramInfoLog", (void**)&glGetProgramInfoLog },
  { "glGetProgramiv", (void**)&glGetProgramiv },
  { "glGetQueryObjecti64v", (void**)&glGetQueryObjecti64v },
  { "glGetQueryObjectiv", (void**)&glGetQueryObjectiv },
  { "glGetQueryObjectui64v", (void**)&glGetQueryObjectui64v },
  { "glGetQueryObjectuiv", (void**)&glGetQueryObjectuiv },
  { "glGetQueryiv", (void**)&glGetQueryiv },
  { "glGetRenderbufferParameteriv", (void**)&glGetRenderbufferParameteriv },
  { "glGetSamplerParameterIiv", (void**)&glGetSamplerParameterIiv },
  { "glGetSamplerParameterIuiv", (void**)&glGetSamplerParameterIuiv },
  { "glGetSamplerParameterfv", (void**)&glGetSamplerParameterfv },
  { "glGetSamplerParameteriv", (void**)&glGetSamplerParameteriv },
  { "glGetShaderInfoLog", (void**)&glGetShaderInfoLog },
  { "glGetShaderSource", (void**)&glGetShaderSource },
  { "glGetShaderiv", (void**)&glGetShaderiv },
  { "glGetString", (void**)&glGetString },
  { "glGetStringi", (void**)&glGetStringi },
  { "glGetSynciv", (void**)&glGetSynciv },
  { "glGetTexImage", (void**)&glGetTexImage },
  { "glGetTexLevelParameterfv", (void**)&glGetTexLevelParameterfv },
  { "glGetTexLevelParameteriv", (void**)&glGetTexLevelParameteriv },
  { "glGetTexParameterIiv", (void**)&glGetTexParameterIiv },
  { "glGetTexParameterIuiv", (void**)&glGetTexParameterIuiv },
  { "glGetTexParameterfv", (void**)&glGetTexParameterfv },
  { "glGetTexParameteriv", (void**)&glGetTexParameteriv },
  { "glGetTransformFeedbackVarying", (void**)&glGetTransformFeedbackVarying },
  { "glGetUniformBlockIndex", (void**)&glGetUniformBlockIndex },
  { "glGetUniformIndices", (void**)&glGetUniformIndices },
  { "glGetUniformLocation", (void**)&glGetUniformLocation },
  { "glGetUniformfv", (void**)&glGetUniformfv },
  { "glGetUniformiv", (void**)&glGetUniformiv },
  { "glGetUniformuiv", (void**)&glGetUniformuiv },
  { "glGetVertexAttribIiv", (void**)&glGetVertexAttribIiv },
  { "glGetVertexAttribIuiv", (void**)&glGetVertexAttribIuiv },
  { "glGetVertexAttribPointerv", (void**)&glGetVertexAttribPointerv },
  { "glGetVertexAttribdv", (void**)&glGetVertexAttribdv },
  { "glGetVertexAttribfv", (void**)&glGetVertexAttribfv },
  { "glGetVertexAttribiv", (void**)&glGetVertexAttribiv },
  { "glHint", (void**)&glHint },
  { "glIsBuffer", (void**)&glIsBuffer },
  { "glIsEnabled", (void**)&glIsEnabled },
  { "glIsEnabledi", (void**)&glIsEnabledi },
  { "glIsFramebuffer", (void**)&glIsFramebuffer },
  { "glIsProgram", (void**)&glIsProgram },
  { "glIsQuery", (void**)&glIsQuery },
  { "glIsRenderbuffer", (void**)&glIsRenderbuffer },
  { "glIsSampler", (void**)&glIsSampler },
  { "glIsShader", (void**)&glIsShader },
  { "glIsSync", (void**)&glIsSync },
  { "glIsTexture", (void**)&glIsTexture },
  { "glIsVertexArray", (void**)&glIsVertexArray },
  { "glLineWidth", (void**)&glLineWidth },
  { "glLinkProgram", (void**)&glLinkProgram },
  { "glLogicOp", (void**)&glLogicOp },
  { "glMapBuffer", (void**)&glMapBuffer },
  { "glMapBufferRange", (void**)&glMapBufferRange },
  { "glMultiDrawArrays", (void**)&glMultiDrawArrays },
  { "glMultiDrawElements", (void**)&glMultiDrawElements },
  { "glMultiDrawElementsBaseVertex", (void**)&glMultiDrawElementsBaseVertex },
  { "glPixelStoref", (void**)&glPixelStoref },
  { "glPixelStorei", (void**)&glPixelStorei },
  { "glPointParameterf", (void**)&glPointParameterf },
  { "glPointParameterfv", (void**)&glPointParameterfv },
  { "glPointParameteri", (void**)&glPointParameteri },
  { "glPointParameteriv", (void**)&glPointParameteriv },
  { "glPointSize", (void**)&glPointSize },
  { "glPolygonMode", (void**)&glPolygonMode },
  { "glPolygonOffset", (void**)&glPolygonOffset },
  { "glPrimitiveRestartIndex", (void**)&glPrimitiveRestartIndex },
  { "glProvokingVertex", (void**)&glProvokingVertex },
  { "glQueryCounter", (void**)&glQueryCounter },
  { "glReadBuffer", (void**)&glReadBuffer },
  { "glReadPixels", (void**)&glReadPixels },
  { "glRenderbufferStorage", (void**)&glRenderbufferStorage },
  { "glRenderbufferStorageMultisample", (void**)&glRenderbufferStorageMultisample },
  { "glSampleCoverage", (void**)&glSampleCoverage },
  { "glSampleMaski", (void**)&glSampleMaski },
  { "glSamplerParameterIiv", (void**)&glSamplerParameterIiv },
  { "glSamplerParameterIuiv", (void**)&glSamplerParameterIuiv },
  { "glSamplerParameterf", (void**)&glSamplerParameterf },
  { "glSamplerParameterfv", (void**)&glSamplerParameterfv },
  { "glSamplerParameteri", (void**)&glSamplerParameteri },
  { "glSamplerParameteriv", (void**)&glSamplerParameteriv },
  { "glScissor", (void**)&glScissor },
  { "glShaderSource", (void**)&glShaderSource },
  { "glStencilFunc", (void**)&glStencilFunc },
  { "glStencilFuncSeparate", (void**)&glStencilFuncSeparate },
  { "glStencilMask", (void**)&glStencilMask },
  { "glStencilMaskSeparate", (void**)&glStencilMaskSeparate },
  { "glStencilOp", (void**)&glStencilOp },
  { "glStencilOpSeparate", (void**)&glStencilOpSeparate },
  { "glTexBuffer", (void**)&glTexBuffer },
  { "glTexImage1D", (void**)&glTexImage1D },
  { "glTexImage2D", (void**)&glTexImage2D },
  { "glTexImage2DMultisample", (void**)&glTexImage2DMultisample },
  { "glTexImage3D", (void**)&glTexImage3D },
  { "glTexImage3DMultisample", (void**)&glTexImage3DMultisample },
  { "glTexParameterIiv", (void**)&glTexParameterIiv },
  { "glTexParameterIuiv", (void**)&glTexParameterIuiv },
  { "glTexParameterf", (void**)&glTexParameterf },
  { "glTexParameterfv", (void**)&glTexParameterfv },
  { "glTexParameteri", (void**)&glTexParameteri },
  { "glTexParameteriv", (void**)&glTexParameteriv },
  { "glTexSubImage1D", (void**)&glTexSubImage1D },
  { "glTexSubImage2D", (void**)&glTexSubImage2D },
  { "glTexSubImage3D", (void**)&glTexSubImage3D },
  { "glTransformFeedbackVaryings", (void**)&glTransformFeedbackVaryings },
  { "glUniform1f", (void**)&glUniform1f },
  { "glUniform1fv", (void**)&glUniform1fv },
  { "glUniform1i", (void**)&glUniform1i },
  { "glUniform1iv", (void**)&glUniform1iv },
  { "glUniform1ui", (void**)&glUniform1ui },
  { "glUniform1uiv", (void**)&glUniform1uiv },
  { "glUniform2f", (void**)&glUniform2f },
  { "glUniform2fv", (void**)&glUniform2fv },
  { "glUniform2i", (void**)&glUniform2i },
  { "glUniform2iv", (void**)&glUniform2iv },
  { "glUniform2ui", (void**)&glUniform2ui },
  { "glUniform2uiv", (void**)&glUniform2uiv },
  { "glUniform3f", (void**)&glUniform3f },
  { "glUniform3fv", (void**)&glUniform3fv },
  { "glUniform3i", (void**)&glUniform3i },
  { "glUniform3iv", (void**)&glUniform3iv },
  { "glUniform3ui", (void**)&glUniform3ui },
  { "glUniform3uiv", (void**)&glUniform3uiv },
  { "glUniform4f", (void**)&glUniform4f },
  { "glUniform4fv", (void**)&glUniform4fv },
  { "glUniform4i", (void**)&glUniform4i },
  { "glUniform4iv", (void**)&glUniform4iv },
  { "glUniform4ui", (void**)&glUniform4ui },
  { "glUniform4uiv", (void**)&glUniform4uiv },
  { "glUniformBlockBinding", (void**)&glUniformBlockBinding },
  { "glUniformMatrix2fv", (void**)&glUniformMatrix2fv },
  { "glUniformMatrix2x3fv", (void**)&glUniformMatrix2x3fv },
  { "glUniformMatrix2x4fv", (void**)&glUniformMatrix2x4fv },
  { "glUniformMatrix3fv", (void**)&glUniformMatrix3fv },
  { "glUniformMatrix3x2fv", (void**)&glUniformMatrix3x2fv },
  { "glUniformMatrix3x4fv", (void**)&glUniformMatrix3x4fv },
  { "glUniformMatrix4fv", (void**)&glUniformMatrix4fv },
  { "glUniformMatrix4x2fv", (void**)&glUniformMatrix4x2fv },
  { "glUniformMatrix4x3fv", (void**)&glUniformMatrix4x3fv },
  { "glUnmapBuffer", (void**)&glUnmapBuffer },
  { "glUseProgram", (void**)&glUseProgram },
  { "glValidateProgram", (void**)&glValidateProgram },
  { "glVertexAttrib1d", (void**)&glVertexAttrib1d },
  { "glVertexAttrib1dv", (void**)&glVertexAttrib1dv },
  { "glVertexAttrib1f", (void**)&glVertexAttrib1f },
  { "glVertexAttrib1fv", (void**)&glVertexAttrib1fv },
  { "glVertexAttrib1s", (void**)&glVertexAttrib1s },
  { "glVertexAttrib1sv", (void**)&glVertexAttrib1sv },
  { "glVertexAttrib2d", (void**)&glVertexAttrib2d },
  { "glVertexAttrib2dv", (void**)&glVertexAttrib2dv },
  { "glVertexAttrib2f", (void**)&glVertexAttrib2f },
  { "glVertexAttrib2fv", (void**)&glVertexAttrib2fv },
  { "glVertexAttrib2s", (void**)&glVertexAttrib2s },
  { "glVertexAttrib2sv", (void**)&glVertexAttrib2sv },
  { "glVertexAttrib3d", (void**)&glVertexAttrib3d },
  { "glVertexAttrib3dv", (void**)&glVertexAttrib3dv },
  { "glVertexAttrib3f", (void**)&glVertexAttrib3f },
  { "glVertexAttrib3fv", (void**)&glVertexAttrib3fv },
  { "glVertexAttrib3s", (void**)&glVertexAttrib3s },
  { "glVertexAttrib3sv", (void**)&glVertexAttrib3sv },
  { "glVertexAttrib4Nbv", (void**)&glVertexAttrib4Nbv },
  { "glVertexAttrib4Niv", (void**)&glVertexAttrib4Niv },
  { "glVertexAttrib4Nsv", (void**)&glVertexAttrib4Nsv },
  { "glVertexAttrib4Nub", (void**)&glVertexAttrib4Nub },
  { "glVertexAttrib4Nubv", (void**)&glVertexAttrib4Nubv },
  { "glVertexAttrib4Nuiv", (void**)&glVertexAttrib4Nuiv },
  { "glVertexAttrib4Nusv", (void**)&glVertexAttrib4Nusv },
  { "glVertexAttrib4bv", (void**)&glVertexAttrib4bv },
  { "glVertexAttrib4d", (void**)&glVertexAttrib4d },
  { "glVertexAttrib4dv", (void**)&glVertexAttrib4dv },
  { "glVertexAttrib4f", (void**)&glVertexAttrib4f },
  { "glVertexAttrib4fv", (void**)&glVertexAttrib4fv },
  { "glVertexAttrib4iv", (void**)&glVertexAttrib4iv },
  { "glVertexAttrib4s", (void**)&glVertexAttrib4s },
  { "glVertexAttrib4sv", (void**)&glVertexAttrib4sv },
  { "glVertexAttrib4ubv", (void**)&glVertexAttrib4ubv },
  { "glVertexAttrib4uiv", (void**)&glVertexAttrib4uiv },
  { "glVertexAttrib4usv", (void**)&glVertexAttrib4usv },
  { "glVertexAttribDivisor", (void**)&glVertexAttribDivisor },
  { "glVertexAttribI1i", (void**)&glVertexAttribI1i },
  { "glVertexAttribI1iv", (void**)&glVertexAttribI1iv },
  { "glVertexAttribI1ui", (void**)&glVertexAttribI1ui },
  { "glVertexAttribI1uiv", (void**)&glVertexAttribI1uiv },
  { "glVertexAttribI2i", (void**)&glVertexAttribI2i },
  { "glVertexAttribI2iv", (void**)&glVertexAttribI2iv },
  { "glVertexAttribI2ui", (void**)&glVertexAttribI2ui },
  { "glVertexAttribI2uiv", (void**)&glVertexAttribI2uiv },
  { "glVertexAttribI3i", (void**)&glVertexAttribI3i },
  { "glVertexAttribI3iv", (void**)&glVertexAttribI3iv },
  { "glVertexAttribI3ui", (void**)&glVertexAttribI3ui },
  { "glVertexAttribI3uiv", (void**)&glVertexAttribI3uiv },
  { "glVertexAttribI4bv", (void**)&glVertexAttribI4bv },
  { "glVertexAttribI4i", (void**)&glVertexAttribI4i },
  { "glVertexAttribI4iv", (void**)&glVertexAttribI4iv },
  { "glVertexAttribI4sv", (void**)&glVertexAttribI4sv },
  { "glVertexAttribI4ubv", (void**)&glVertexAttribI4ubv },
  { "glVertexAttribI4ui", (void**)&glVertexAttribI4ui },
  { "glVertexAttribI4uiv", (void**)&glVertexAttribI4uiv },
  { "glVertexAttribI4usv", (void**)&glVertexAttribI4usv },
  { "glVertexAttribIPointer", (void**)&glVertexAttribIPointer },
  { "glVertexAttribP1ui", (void**)&glVertexAttribP1ui },
  { "glVertexAttribP1uiv", (void**)&glVertexAttribP1uiv },
  { "glVertexAttribP2ui", (void**)&glVertexAttribP2ui },
  { "glVertexAttribP2uiv", (void**)&glVertexAttribP2uiv },
  { "glVertexAttribP3ui", (void**)&glVertexAttribP3ui },
  { "glVertexAttribP3uiv", (void**)&glVertexAttribP3uiv },
  { "glVertexAttribP4ui", (void**)&glVertexAttribP4ui },
  { "glVertexAttribP4uiv", (void**)&glVertexAttribP4uiv },
  { "glVertexAttribPointer", (void**)&glVertexAttribPointer },
  { "glViewport", (void**)&glViewport },
  { "glWaitSync", (void**)&glWaitSync },
};

// EN: Function-pointer-typed cast of a void* from dlsym/GetProcAddress via a union
//     (not a direct reinterpret/C-style cast) -- POSIX explicitly sanctions this
//     idiom (see the dlsym(3) rationale: ISO C forbids converting object pointers
//     to function pointers directly, but the round-trip through a union member is
//     well-defined on POSIX platforms and avoids -Wpedantic / strict-aliasing
//     complaints).
// PT: Cast tipado-para-ponteiro-de-função de um void* vindo de dlsym/GetProcAddress
//     via union (não um reinterpret/C-style cast direto) -- o POSIX sanciona
//     explicitamente esse idioma (ver o racional de dlsym(3): o ISO C proíbe
//     converter ponteiro de objeto para ponteiro de função diretamente, mas o
//     round-trip via membro de union é bem definido em plataformas POSIX e evita
//     avisos de -Wpedantic / strict-aliasing).
typedef void (*glintfx_glx_fn_t)(void);

typedef glintfx_glx_fn_t (*glintfx_glXGetProcAddress_t)(const unsigned char*);
typedef void* (*glintfx_eglGetProcAddress_t)(const char*);

static void* glintfx_gl_lib_handle = NULL;
static glintfx_glXGetProcAddress_t glintfx_glx_get_proc_address = NULL;
static glintfx_eglGetProcAddress_t glintfx_egl_get_proc_address = NULL;

static void* glintfx_resolve_symbol(const char* name) {
  union { void* obj; glintfx_glx_fn_t fn; } cast;
  cast.obj = NULL;

  if (glintfx_glx_get_proc_address) {
    cast.fn = glintfx_glx_get_proc_address((const unsigned char*)name);
    if (cast.obj) return cast.obj;
  }
  if (glintfx_egl_get_proc_address) {
    cast.obj = glintfx_egl_get_proc_address(name);
    if (cast.obj) return cast.obj;
  }
  if (glintfx_gl_lib_handle) {
    cast.obj = dlsym(glintfx_gl_lib_handle, name);
    if (cast.obj) return cast.obj;
  }
  return NULL;
}

int glx_gl_load(void) {
  // EN: dlopen libGL.so.1 first (SONAME on virtually every Linux distro's Mesa/
  //     proprietary-driver package); fall back to the unversioned libGL.so (dev
  //     package name) for hosts missing the versioned symlink. RTLD_GLOBAL so the
  //     symbols are visible for the eglGetProcAddress fallback path below too.
  // PT: Abre libGL.so.1 primeiro (SONAME em praticamente toda distro Linux com
  //     Mesa/driver proprietário); cai para libGL.so sem versão (nome do pacote
  //     -dev) em hosts sem o symlink versionado. RTLD_GLOBAL para que os símbolos
  //     fiquem visíveis também para o caminho de fallback eglGetProcAddress abaixo.
  if (!glintfx_gl_lib_handle) {
    glintfx_gl_lib_handle = dlopen("libGL.so.1", RTLD_NOW | RTLD_GLOBAL);
    if (!glintfx_gl_lib_handle) {
      glintfx_gl_lib_handle = dlopen("libGL.so", RTLD_NOW | RTLD_GLOBAL);
    }
  }

  if (glintfx_gl_lib_handle && !glintfx_glx_get_proc_address) {
    union { void* obj; glintfx_glXGetProcAddress_t fn; } cast;
    cast.obj = dlsym(glintfx_gl_lib_handle, "glXGetProcAddressARB");
    if (!cast.obj) cast.obj = dlsym(glintfx_gl_lib_handle, "glXGetProcAddress");
    glintfx_glx_get_proc_address = cast.fn;
  }

  if (!glintfx_egl_get_proc_address) {
    void* egl_handle = dlopen("libEGL.so.1", RTLD_NOW | RTLD_GLOBAL);
    if (!egl_handle) egl_handle = dlopen("libEGL.so", RTLD_NOW | RTLD_GLOBAL);
    if (egl_handle) {
      union { void* obj; glintfx_eglGetProcAddress_t fn; } cast;
      cast.obj = dlsym(egl_handle, "eglGetProcAddress");
      glintfx_egl_get_proc_address = cast.fn;
    }
  }

  int missing = 0;
  for (size_t i = 0; i < sizeof(glintfx_gl_symbol_table) / sizeof(glintfx_gl_symbol_table[0]); ++i) {
    void* resolved = glintfx_resolve_symbol(glintfx_gl_symbol_table[i].name);
    *glintfx_gl_symbol_table[i].slot = resolved;
    if (!resolved) ++missing;
  }
  return missing ? 1 : 0;
}

#ifdef __cplusplus
}
#endif
