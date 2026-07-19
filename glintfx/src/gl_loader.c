// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 Petrus Silva Costa
//
// EN: GENERATED FILE -- do not hand-edit. See gl_loader.h / tools/gen_glloader.py.
//
//     Every pointer variable defined below carries the `glx_` prefix (AUD-L1-GLSYM,
//     2026-07-19, e.g. `glx_glCullFace`); the symbol table keys on the BARE GL name
//     (`"glCullFace"`) because that is the literal string GetProcAddress needs to
//     resolve the real driver entry point -- a string literal is not an identifier and
//     is therefore never touched by gl_loader.h's `#define glCullFace glx_glCullFace`
//     call-site-rewrite macros, so there is no risk of this table ending up keyed on
//     the wrong (prefixed) string.
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
//     Toda variável-ponteiro definida abaixo carrega o prefixo `glx_` (AUD-L1-GLSYM,
//     2026-07-19, ex.: `glx_glCullFace`); a tabela de símbolos é indexada pelo nome GL
//     CRU (`"glCullFace"`) porque é essa a string literal que o GetProcAddress precisa
//     para resolver o entry point real do driver -- um literal de string não é um
//     identificador e por isso nunca é tocado pelas macros de reescrita de call site
//     `#define glCullFace glx_glCullFace` de gl_loader.h, então não há risco de esta
//     tabela acabar indexada pela string (prefixada) errada.
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

#include <stddef.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

PFNGLACTIVETEXTUREPROC glx_glActiveTexture = NULL;
PFNGLATTACHSHADERPROC glx_glAttachShader = NULL;
PFNGLBEGINCONDITIONALRENDERPROC glx_glBeginConditionalRender = NULL;
PFNGLBEGINQUERYPROC glx_glBeginQuery = NULL;
PFNGLBEGINTRANSFORMFEEDBACKPROC glx_glBeginTransformFeedback = NULL;
PFNGLBINDATTRIBLOCATIONPROC glx_glBindAttribLocation = NULL;
PFNGLBINDBUFFERPROC glx_glBindBuffer = NULL;
PFNGLBINDBUFFERBASEPROC glx_glBindBufferBase = NULL;
PFNGLBINDBUFFERRANGEPROC glx_glBindBufferRange = NULL;
PFNGLBINDFRAGDATALOCATIONPROC glx_glBindFragDataLocation = NULL;
PFNGLBINDFRAGDATALOCATIONINDEXEDPROC glx_glBindFragDataLocationIndexed = NULL;
PFNGLBINDFRAMEBUFFERPROC glx_glBindFramebuffer = NULL;
PFNGLBINDRENDERBUFFERPROC glx_glBindRenderbuffer = NULL;
PFNGLBINDSAMPLERPROC glx_glBindSampler = NULL;
PFNGLBINDTEXTUREPROC glx_glBindTexture = NULL;
PFNGLBINDVERTEXARRAYPROC glx_glBindVertexArray = NULL;
PFNGLBLENDCOLORPROC glx_glBlendColor = NULL;
PFNGLBLENDEQUATIONPROC glx_glBlendEquation = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC glx_glBlendEquationSeparate = NULL;
PFNGLBLENDFUNCPROC glx_glBlendFunc = NULL;
PFNGLBLENDFUNCSEPARATEPROC glx_glBlendFuncSeparate = NULL;
PFNGLBLITFRAMEBUFFERPROC glx_glBlitFramebuffer = NULL;
PFNGLBUFFERDATAPROC glx_glBufferData = NULL;
PFNGLBUFFERSUBDATAPROC glx_glBufferSubData = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glx_glCheckFramebufferStatus = NULL;
PFNGLCLAMPCOLORPROC glx_glClampColor = NULL;
PFNGLCLEARPROC glx_glClear = NULL;
PFNGLCLEARBUFFERFIPROC glx_glClearBufferfi = NULL;
PFNGLCLEARBUFFERFVPROC glx_glClearBufferfv = NULL;
PFNGLCLEARBUFFERIVPROC glx_glClearBufferiv = NULL;
PFNGLCLEARBUFFERUIVPROC glx_glClearBufferuiv = NULL;
PFNGLCLEARCOLORPROC glx_glClearColor = NULL;
PFNGLCLEARDEPTHPROC glx_glClearDepth = NULL;
PFNGLCLEARSTENCILPROC glx_glClearStencil = NULL;
PFNGLCLIENTWAITSYNCPROC glx_glClientWaitSync = NULL;
PFNGLCOLORMASKPROC glx_glColorMask = NULL;
PFNGLCOLORMASKIPROC glx_glColorMaski = NULL;
PFNGLCOMPILESHADERPROC glx_glCompileShader = NULL;
PFNGLCOMPRESSEDTEXIMAGE1DPROC glx_glCompressedTexImage1D = NULL;
PFNGLCOMPRESSEDTEXIMAGE2DPROC glx_glCompressedTexImage2D = NULL;
PFNGLCOMPRESSEDTEXIMAGE3DPROC glx_glCompressedTexImage3D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glx_glCompressedTexSubImage1D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glx_glCompressedTexSubImage2D = NULL;
PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glx_glCompressedTexSubImage3D = NULL;
PFNGLCOPYBUFFERSUBDATAPROC glx_glCopyBufferSubData = NULL;
PFNGLCOPYTEXIMAGE1DPROC glx_glCopyTexImage1D = NULL;
PFNGLCOPYTEXIMAGE2DPROC glx_glCopyTexImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE1DPROC glx_glCopyTexSubImage1D = NULL;
PFNGLCOPYTEXSUBIMAGE2DPROC glx_glCopyTexSubImage2D = NULL;
PFNGLCOPYTEXSUBIMAGE3DPROC glx_glCopyTexSubImage3D = NULL;
PFNGLCREATEPROGRAMPROC glx_glCreateProgram = NULL;
PFNGLCREATESHADERPROC glx_glCreateShader = NULL;
PFNGLCULLFACEPROC glx_glCullFace = NULL;
PFNGLDELETEBUFFERSPROC glx_glDeleteBuffers = NULL;
PFNGLDELETEFRAMEBUFFERSPROC glx_glDeleteFramebuffers = NULL;
PFNGLDELETEPROGRAMPROC glx_glDeleteProgram = NULL;
PFNGLDELETEQUERIESPROC glx_glDeleteQueries = NULL;
PFNGLDELETERENDERBUFFERSPROC glx_glDeleteRenderbuffers = NULL;
PFNGLDELETESAMPLERSPROC glx_glDeleteSamplers = NULL;
PFNGLDELETESHADERPROC glx_glDeleteShader = NULL;
PFNGLDELETESYNCPROC glx_glDeleteSync = NULL;
PFNGLDELETETEXTURESPROC glx_glDeleteTextures = NULL;
PFNGLDELETEVERTEXARRAYSPROC glx_glDeleteVertexArrays = NULL;
PFNGLDEPTHFUNCPROC glx_glDepthFunc = NULL;
PFNGLDEPTHMASKPROC glx_glDepthMask = NULL;
PFNGLDEPTHRANGEPROC glx_glDepthRange = NULL;
PFNGLDETACHSHADERPROC glx_glDetachShader = NULL;
PFNGLDISABLEPROC glx_glDisable = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glx_glDisableVertexAttribArray = NULL;
PFNGLDISABLEIPROC glx_glDisablei = NULL;
PFNGLDRAWARRAYSPROC glx_glDrawArrays = NULL;
PFNGLDRAWARRAYSINSTANCEDPROC glx_glDrawArraysInstanced = NULL;
PFNGLDRAWBUFFERPROC glx_glDrawBuffer = NULL;
PFNGLDRAWBUFFERSPROC glx_glDrawBuffers = NULL;
PFNGLDRAWELEMENTSPROC glx_glDrawElements = NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC glx_glDrawElementsBaseVertex = NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC glx_glDrawElementsInstanced = NULL;
PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glx_glDrawElementsInstancedBaseVertex = NULL;
PFNGLDRAWRANGEELEMENTSPROC glx_glDrawRangeElements = NULL;
PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC glx_glDrawRangeElementsBaseVertex = NULL;
PFNGLENABLEPROC glx_glEnable = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC glx_glEnableVertexAttribArray = NULL;
PFNGLENABLEIPROC glx_glEnablei = NULL;
PFNGLENDCONDITIONALRENDERPROC glx_glEndConditionalRender = NULL;
PFNGLENDQUERYPROC glx_glEndQuery = NULL;
PFNGLENDTRANSFORMFEEDBACKPROC glx_glEndTransformFeedback = NULL;
PFNGLFENCESYNCPROC glx_glFenceSync = NULL;
PFNGLFINISHPROC glx_glFinish = NULL;
PFNGLFLUSHPROC glx_glFlush = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC glx_glFlushMappedBufferRange = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glx_glFramebufferRenderbuffer = NULL;
PFNGLFRAMEBUFFERTEXTUREPROC glx_glFramebufferTexture = NULL;
PFNGLFRAMEBUFFERTEXTURE1DPROC glx_glFramebufferTexture1D = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glx_glFramebufferTexture2D = NULL;
PFNGLFRAMEBUFFERTEXTURE3DPROC glx_glFramebufferTexture3D = NULL;
PFNGLFRAMEBUFFERTEXTURELAYERPROC glx_glFramebufferTextureLayer = NULL;
PFNGLFRONTFACEPROC glx_glFrontFace = NULL;
PFNGLGENBUFFERSPROC glx_glGenBuffers = NULL;
PFNGLGENFRAMEBUFFERSPROC glx_glGenFramebuffers = NULL;
PFNGLGENQUERIESPROC glx_glGenQueries = NULL;
PFNGLGENRENDERBUFFERSPROC glx_glGenRenderbuffers = NULL;
PFNGLGENSAMPLERSPROC glx_glGenSamplers = NULL;
PFNGLGENTEXTURESPROC glx_glGenTextures = NULL;
PFNGLGENVERTEXARRAYSPROC glx_glGenVertexArrays = NULL;
PFNGLGENERATEMIPMAPPROC glx_glGenerateMipmap = NULL;
PFNGLGETACTIVEATTRIBPROC glx_glGetActiveAttrib = NULL;
PFNGLGETACTIVEUNIFORMPROC glx_glGetActiveUniform = NULL;
PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC glx_glGetActiveUniformBlockName = NULL;
PFNGLGETACTIVEUNIFORMBLOCKIVPROC glx_glGetActiveUniformBlockiv = NULL;
PFNGLGETACTIVEUNIFORMNAMEPROC glx_glGetActiveUniformName = NULL;
PFNGLGETACTIVEUNIFORMSIVPROC glx_glGetActiveUniformsiv = NULL;
PFNGLGETATTACHEDSHADERSPROC glx_glGetAttachedShaders = NULL;
PFNGLGETATTRIBLOCATIONPROC glx_glGetAttribLocation = NULL;
PFNGLGETBOOLEANI_VPROC glx_glGetBooleani_v = NULL;
PFNGLGETBOOLEANVPROC glx_glGetBooleanv = NULL;
PFNGLGETBUFFERPARAMETERI64VPROC glx_glGetBufferParameteri64v = NULL;
PFNGLGETBUFFERPARAMETERIVPROC glx_glGetBufferParameteriv = NULL;
PFNGLGETBUFFERPOINTERVPROC glx_glGetBufferPointerv = NULL;
PFNGLGETBUFFERSUBDATAPROC glx_glGetBufferSubData = NULL;
PFNGLGETCOMPRESSEDTEXIMAGEPROC glx_glGetCompressedTexImage = NULL;
PFNGLGETDOUBLEVPROC glx_glGetDoublev = NULL;
PFNGLGETERRORPROC glx_glGetError = NULL;
PFNGLGETFLOATVPROC glx_glGetFloatv = NULL;
PFNGLGETFRAGDATAINDEXPROC glx_glGetFragDataIndex = NULL;
PFNGLGETFRAGDATALOCATIONPROC glx_glGetFragDataLocation = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glx_glGetFramebufferAttachmentParameteriv = NULL;
PFNGLGETINTEGER64I_VPROC glx_glGetInteger64i_v = NULL;
PFNGLGETINTEGER64VPROC glx_glGetInteger64v = NULL;
PFNGLGETINTEGERI_VPROC glx_glGetIntegeri_v = NULL;
PFNGLGETINTEGERVPROC glx_glGetIntegerv = NULL;
PFNGLGETMULTISAMPLEFVPROC glx_glGetMultisamplefv = NULL;
PFNGLGETPROGRAMINFOLOGPROC glx_glGetProgramInfoLog = NULL;
PFNGLGETPROGRAMIVPROC glx_glGetProgramiv = NULL;
PFNGLGETQUERYOBJECTI64VPROC glx_glGetQueryObjecti64v = NULL;
PFNGLGETQUERYOBJECTIVPROC glx_glGetQueryObjectiv = NULL;
PFNGLGETQUERYOBJECTUI64VPROC glx_glGetQueryObjectui64v = NULL;
PFNGLGETQUERYOBJECTUIVPROC glx_glGetQueryObjectuiv = NULL;
PFNGLGETQUERYIVPROC glx_glGetQueryiv = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVPROC glx_glGetRenderbufferParameteriv = NULL;
PFNGLGETSAMPLERPARAMETERIIVPROC glx_glGetSamplerParameterIiv = NULL;
PFNGLGETSAMPLERPARAMETERIUIVPROC glx_glGetSamplerParameterIuiv = NULL;
PFNGLGETSAMPLERPARAMETERFVPROC glx_glGetSamplerParameterfv = NULL;
PFNGLGETSAMPLERPARAMETERIVPROC glx_glGetSamplerParameteriv = NULL;
PFNGLGETSHADERINFOLOGPROC glx_glGetShaderInfoLog = NULL;
PFNGLGETSHADERSOURCEPROC glx_glGetShaderSource = NULL;
PFNGLGETSHADERIVPROC glx_glGetShaderiv = NULL;
PFNGLGETSTRINGPROC glx_glGetString = NULL;
PFNGLGETSTRINGIPROC glx_glGetStringi = NULL;
PFNGLGETSYNCIVPROC glx_glGetSynciv = NULL;
PFNGLGETTEXIMAGEPROC glx_glGetTexImage = NULL;
PFNGLGETTEXLEVELPARAMETERFVPROC glx_glGetTexLevelParameterfv = NULL;
PFNGLGETTEXLEVELPARAMETERIVPROC glx_glGetTexLevelParameteriv = NULL;
PFNGLGETTEXPARAMETERIIVPROC glx_glGetTexParameterIiv = NULL;
PFNGLGETTEXPARAMETERIUIVPROC glx_glGetTexParameterIuiv = NULL;
PFNGLGETTEXPARAMETERFVPROC glx_glGetTexParameterfv = NULL;
PFNGLGETTEXPARAMETERIVPROC glx_glGetTexParameteriv = NULL;
PFNGLGETTRANSFORMFEEDBACKVARYINGPROC glx_glGetTransformFeedbackVarying = NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC glx_glGetUniformBlockIndex = NULL;
PFNGLGETUNIFORMINDICESPROC glx_glGetUniformIndices = NULL;
PFNGLGETUNIFORMLOCATIONPROC glx_glGetUniformLocation = NULL;
PFNGLGETUNIFORMFVPROC glx_glGetUniformfv = NULL;
PFNGLGETUNIFORMIVPROC glx_glGetUniformiv = NULL;
PFNGLGETUNIFORMUIVPROC glx_glGetUniformuiv = NULL;
PFNGLGETVERTEXATTRIBIIVPROC glx_glGetVertexAttribIiv = NULL;
PFNGLGETVERTEXATTRIBIUIVPROC glx_glGetVertexAttribIuiv = NULL;
PFNGLGETVERTEXATTRIBPOINTERVPROC glx_glGetVertexAttribPointerv = NULL;
PFNGLGETVERTEXATTRIBDVPROC glx_glGetVertexAttribdv = NULL;
PFNGLGETVERTEXATTRIBFVPROC glx_glGetVertexAttribfv = NULL;
PFNGLGETVERTEXATTRIBIVPROC glx_glGetVertexAttribiv = NULL;
PFNGLHINTPROC glx_glHint = NULL;
PFNGLISBUFFERPROC glx_glIsBuffer = NULL;
PFNGLISENABLEDPROC glx_glIsEnabled = NULL;
PFNGLISENABLEDIPROC glx_glIsEnabledi = NULL;
PFNGLISFRAMEBUFFERPROC glx_glIsFramebuffer = NULL;
PFNGLISPROGRAMPROC glx_glIsProgram = NULL;
PFNGLISQUERYPROC glx_glIsQuery = NULL;
PFNGLISRENDERBUFFERPROC glx_glIsRenderbuffer = NULL;
PFNGLISSAMPLERPROC glx_glIsSampler = NULL;
PFNGLISSHADERPROC glx_glIsShader = NULL;
PFNGLISSYNCPROC glx_glIsSync = NULL;
PFNGLISTEXTUREPROC glx_glIsTexture = NULL;
PFNGLISVERTEXARRAYPROC glx_glIsVertexArray = NULL;
PFNGLLINEWIDTHPROC glx_glLineWidth = NULL;
PFNGLLINKPROGRAMPROC glx_glLinkProgram = NULL;
PFNGLLOGICOPPROC glx_glLogicOp = NULL;
PFNGLMAPBUFFERPROC glx_glMapBuffer = NULL;
PFNGLMAPBUFFERRANGEPROC glx_glMapBufferRange = NULL;
PFNGLMULTIDRAWARRAYSPROC glx_glMultiDrawArrays = NULL;
PFNGLMULTIDRAWELEMENTSPROC glx_glMultiDrawElements = NULL;
PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC glx_glMultiDrawElementsBaseVertex = NULL;
PFNGLPIXELSTOREFPROC glx_glPixelStoref = NULL;
PFNGLPIXELSTOREIPROC glx_glPixelStorei = NULL;
PFNGLPOINTPARAMETERFPROC glx_glPointParameterf = NULL;
PFNGLPOINTPARAMETERFVPROC glx_glPointParameterfv = NULL;
PFNGLPOINTPARAMETERIPROC glx_glPointParameteri = NULL;
PFNGLPOINTPARAMETERIVPROC glx_glPointParameteriv = NULL;
PFNGLPOINTSIZEPROC glx_glPointSize = NULL;
PFNGLPOLYGONMODEPROC glx_glPolygonMode = NULL;
PFNGLPOLYGONOFFSETPROC glx_glPolygonOffset = NULL;
PFNGLPRIMITIVERESTARTINDEXPROC glx_glPrimitiveRestartIndex = NULL;
PFNGLPROVOKINGVERTEXPROC glx_glProvokingVertex = NULL;
PFNGLQUERYCOUNTERPROC glx_glQueryCounter = NULL;
PFNGLREADBUFFERPROC glx_glReadBuffer = NULL;
PFNGLREADPIXELSPROC glx_glReadPixels = NULL;
PFNGLRENDERBUFFERSTORAGEPROC glx_glRenderbufferStorage = NULL;
PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glx_glRenderbufferStorageMultisample = NULL;
PFNGLSAMPLECOVERAGEPROC glx_glSampleCoverage = NULL;
PFNGLSAMPLEMASKIPROC glx_glSampleMaski = NULL;
PFNGLSAMPLERPARAMETERIIVPROC glx_glSamplerParameterIiv = NULL;
PFNGLSAMPLERPARAMETERIUIVPROC glx_glSamplerParameterIuiv = NULL;
PFNGLSAMPLERPARAMETERFPROC glx_glSamplerParameterf = NULL;
PFNGLSAMPLERPARAMETERFVPROC glx_glSamplerParameterfv = NULL;
PFNGLSAMPLERPARAMETERIPROC glx_glSamplerParameteri = NULL;
PFNGLSAMPLERPARAMETERIVPROC glx_glSamplerParameteriv = NULL;
PFNGLSCISSORPROC glx_glScissor = NULL;
PFNGLSHADERSOURCEPROC glx_glShaderSource = NULL;
PFNGLSTENCILFUNCPROC glx_glStencilFunc = NULL;
PFNGLSTENCILFUNCSEPARATEPROC glx_glStencilFuncSeparate = NULL;
PFNGLSTENCILMASKPROC glx_glStencilMask = NULL;
PFNGLSTENCILMASKSEPARATEPROC glx_glStencilMaskSeparate = NULL;
PFNGLSTENCILOPPROC glx_glStencilOp = NULL;
PFNGLSTENCILOPSEPARATEPROC glx_glStencilOpSeparate = NULL;
PFNGLTEXBUFFERPROC glx_glTexBuffer = NULL;
PFNGLTEXIMAGE1DPROC glx_glTexImage1D = NULL;
PFNGLTEXIMAGE2DPROC glx_glTexImage2D = NULL;
PFNGLTEXIMAGE2DMULTISAMPLEPROC glx_glTexImage2DMultisample = NULL;
PFNGLTEXIMAGE3DPROC glx_glTexImage3D = NULL;
PFNGLTEXIMAGE3DMULTISAMPLEPROC glx_glTexImage3DMultisample = NULL;
PFNGLTEXPARAMETERIIVPROC glx_glTexParameterIiv = NULL;
PFNGLTEXPARAMETERIUIVPROC glx_glTexParameterIuiv = NULL;
PFNGLTEXPARAMETERFPROC glx_glTexParameterf = NULL;
PFNGLTEXPARAMETERFVPROC glx_glTexParameterfv = NULL;
PFNGLTEXPARAMETERIPROC glx_glTexParameteri = NULL;
PFNGLTEXPARAMETERIVPROC glx_glTexParameteriv = NULL;
PFNGLTEXSUBIMAGE1DPROC glx_glTexSubImage1D = NULL;
PFNGLTEXSUBIMAGE2DPROC glx_glTexSubImage2D = NULL;
PFNGLTEXSUBIMAGE3DPROC glx_glTexSubImage3D = NULL;
PFNGLTRANSFORMFEEDBACKVARYINGSPROC glx_glTransformFeedbackVaryings = NULL;
PFNGLUNIFORM1FPROC glx_glUniform1f = NULL;
PFNGLUNIFORM1FVPROC glx_glUniform1fv = NULL;
PFNGLUNIFORM1IPROC glx_glUniform1i = NULL;
PFNGLUNIFORM1IVPROC glx_glUniform1iv = NULL;
PFNGLUNIFORM1UIPROC glx_glUniform1ui = NULL;
PFNGLUNIFORM1UIVPROC glx_glUniform1uiv = NULL;
PFNGLUNIFORM2FPROC glx_glUniform2f = NULL;
PFNGLUNIFORM2FVPROC glx_glUniform2fv = NULL;
PFNGLUNIFORM2IPROC glx_glUniform2i = NULL;
PFNGLUNIFORM2IVPROC glx_glUniform2iv = NULL;
PFNGLUNIFORM2UIPROC glx_glUniform2ui = NULL;
PFNGLUNIFORM2UIVPROC glx_glUniform2uiv = NULL;
PFNGLUNIFORM3FPROC glx_glUniform3f = NULL;
PFNGLUNIFORM3FVPROC glx_glUniform3fv = NULL;
PFNGLUNIFORM3IPROC glx_glUniform3i = NULL;
PFNGLUNIFORM3IVPROC glx_glUniform3iv = NULL;
PFNGLUNIFORM3UIPROC glx_glUniform3ui = NULL;
PFNGLUNIFORM3UIVPROC glx_glUniform3uiv = NULL;
PFNGLUNIFORM4FPROC glx_glUniform4f = NULL;
PFNGLUNIFORM4FVPROC glx_glUniform4fv = NULL;
PFNGLUNIFORM4IPROC glx_glUniform4i = NULL;
PFNGLUNIFORM4IVPROC glx_glUniform4iv = NULL;
PFNGLUNIFORM4UIPROC glx_glUniform4ui = NULL;
PFNGLUNIFORM4UIVPROC glx_glUniform4uiv = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC glx_glUniformBlockBinding = NULL;
PFNGLUNIFORMMATRIX2FVPROC glx_glUniformMatrix2fv = NULL;
PFNGLUNIFORMMATRIX2X3FVPROC glx_glUniformMatrix2x3fv = NULL;
PFNGLUNIFORMMATRIX2X4FVPROC glx_glUniformMatrix2x4fv = NULL;
PFNGLUNIFORMMATRIX3FVPROC glx_glUniformMatrix3fv = NULL;
PFNGLUNIFORMMATRIX3X2FVPROC glx_glUniformMatrix3x2fv = NULL;
PFNGLUNIFORMMATRIX3X4FVPROC glx_glUniformMatrix3x4fv = NULL;
PFNGLUNIFORMMATRIX4FVPROC glx_glUniformMatrix4fv = NULL;
PFNGLUNIFORMMATRIX4X2FVPROC glx_glUniformMatrix4x2fv = NULL;
PFNGLUNIFORMMATRIX4X3FVPROC glx_glUniformMatrix4x3fv = NULL;
PFNGLUNMAPBUFFERPROC glx_glUnmapBuffer = NULL;
PFNGLUSEPROGRAMPROC glx_glUseProgram = NULL;
PFNGLVALIDATEPROGRAMPROC glx_glValidateProgram = NULL;
PFNGLVERTEXATTRIB1DPROC glx_glVertexAttrib1d = NULL;
PFNGLVERTEXATTRIB1DVPROC glx_glVertexAttrib1dv = NULL;
PFNGLVERTEXATTRIB1FPROC glx_glVertexAttrib1f = NULL;
PFNGLVERTEXATTRIB1FVPROC glx_glVertexAttrib1fv = NULL;
PFNGLVERTEXATTRIB1SPROC glx_glVertexAttrib1s = NULL;
PFNGLVERTEXATTRIB1SVPROC glx_glVertexAttrib1sv = NULL;
PFNGLVERTEXATTRIB2DPROC glx_glVertexAttrib2d = NULL;
PFNGLVERTEXATTRIB2DVPROC glx_glVertexAttrib2dv = NULL;
PFNGLVERTEXATTRIB2FPROC glx_glVertexAttrib2f = NULL;
PFNGLVERTEXATTRIB2FVPROC glx_glVertexAttrib2fv = NULL;
PFNGLVERTEXATTRIB2SPROC glx_glVertexAttrib2s = NULL;
PFNGLVERTEXATTRIB2SVPROC glx_glVertexAttrib2sv = NULL;
PFNGLVERTEXATTRIB3DPROC glx_glVertexAttrib3d = NULL;
PFNGLVERTEXATTRIB3DVPROC glx_glVertexAttrib3dv = NULL;
PFNGLVERTEXATTRIB3FPROC glx_glVertexAttrib3f = NULL;
PFNGLVERTEXATTRIB3FVPROC glx_glVertexAttrib3fv = NULL;
PFNGLVERTEXATTRIB3SPROC glx_glVertexAttrib3s = NULL;
PFNGLVERTEXATTRIB3SVPROC glx_glVertexAttrib3sv = NULL;
PFNGLVERTEXATTRIB4NBVPROC glx_glVertexAttrib4Nbv = NULL;
PFNGLVERTEXATTRIB4NIVPROC glx_glVertexAttrib4Niv = NULL;
PFNGLVERTEXATTRIB4NSVPROC glx_glVertexAttrib4Nsv = NULL;
PFNGLVERTEXATTRIB4NUBPROC glx_glVertexAttrib4Nub = NULL;
PFNGLVERTEXATTRIB4NUBVPROC glx_glVertexAttrib4Nubv = NULL;
PFNGLVERTEXATTRIB4NUIVPROC glx_glVertexAttrib4Nuiv = NULL;
PFNGLVERTEXATTRIB4NUSVPROC glx_glVertexAttrib4Nusv = NULL;
PFNGLVERTEXATTRIB4BVPROC glx_glVertexAttrib4bv = NULL;
PFNGLVERTEXATTRIB4DPROC glx_glVertexAttrib4d = NULL;
PFNGLVERTEXATTRIB4DVPROC glx_glVertexAttrib4dv = NULL;
PFNGLVERTEXATTRIB4FPROC glx_glVertexAttrib4f = NULL;
PFNGLVERTEXATTRIB4FVPROC glx_glVertexAttrib4fv = NULL;
PFNGLVERTEXATTRIB4IVPROC glx_glVertexAttrib4iv = NULL;
PFNGLVERTEXATTRIB4SPROC glx_glVertexAttrib4s = NULL;
PFNGLVERTEXATTRIB4SVPROC glx_glVertexAttrib4sv = NULL;
PFNGLVERTEXATTRIB4UBVPROC glx_glVertexAttrib4ubv = NULL;
PFNGLVERTEXATTRIB4UIVPROC glx_glVertexAttrib4uiv = NULL;
PFNGLVERTEXATTRIB4USVPROC glx_glVertexAttrib4usv = NULL;
PFNGLVERTEXATTRIBDIVISORPROC glx_glVertexAttribDivisor = NULL;
PFNGLVERTEXATTRIBI1IPROC glx_glVertexAttribI1i = NULL;
PFNGLVERTEXATTRIBI1IVPROC glx_glVertexAttribI1iv = NULL;
PFNGLVERTEXATTRIBI1UIPROC glx_glVertexAttribI1ui = NULL;
PFNGLVERTEXATTRIBI1UIVPROC glx_glVertexAttribI1uiv = NULL;
PFNGLVERTEXATTRIBI2IPROC glx_glVertexAttribI2i = NULL;
PFNGLVERTEXATTRIBI2IVPROC glx_glVertexAttribI2iv = NULL;
PFNGLVERTEXATTRIBI2UIPROC glx_glVertexAttribI2ui = NULL;
PFNGLVERTEXATTRIBI2UIVPROC glx_glVertexAttribI2uiv = NULL;
PFNGLVERTEXATTRIBI3IPROC glx_glVertexAttribI3i = NULL;
PFNGLVERTEXATTRIBI3IVPROC glx_glVertexAttribI3iv = NULL;
PFNGLVERTEXATTRIBI3UIPROC glx_glVertexAttribI3ui = NULL;
PFNGLVERTEXATTRIBI3UIVPROC glx_glVertexAttribI3uiv = NULL;
PFNGLVERTEXATTRIBI4BVPROC glx_glVertexAttribI4bv = NULL;
PFNGLVERTEXATTRIBI4IPROC glx_glVertexAttribI4i = NULL;
PFNGLVERTEXATTRIBI4IVPROC glx_glVertexAttribI4iv = NULL;
PFNGLVERTEXATTRIBI4SVPROC glx_glVertexAttribI4sv = NULL;
PFNGLVERTEXATTRIBI4UBVPROC glx_glVertexAttribI4ubv = NULL;
PFNGLVERTEXATTRIBI4UIPROC glx_glVertexAttribI4ui = NULL;
PFNGLVERTEXATTRIBI4UIVPROC glx_glVertexAttribI4uiv = NULL;
PFNGLVERTEXATTRIBI4USVPROC glx_glVertexAttribI4usv = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC glx_glVertexAttribIPointer = NULL;
PFNGLVERTEXATTRIBP1UIPROC glx_glVertexAttribP1ui = NULL;
PFNGLVERTEXATTRIBP1UIVPROC glx_glVertexAttribP1uiv = NULL;
PFNGLVERTEXATTRIBP2UIPROC glx_glVertexAttribP2ui = NULL;
PFNGLVERTEXATTRIBP2UIVPROC glx_glVertexAttribP2uiv = NULL;
PFNGLVERTEXATTRIBP3UIPROC glx_glVertexAttribP3ui = NULL;
PFNGLVERTEXATTRIBP3UIVPROC glx_glVertexAttribP3uiv = NULL;
PFNGLVERTEXATTRIBP4UIPROC glx_glVertexAttribP4ui = NULL;
PFNGLVERTEXATTRIBP4UIVPROC glx_glVertexAttribP4uiv = NULL;
PFNGLVERTEXATTRIBPOINTERPROC glx_glVertexAttribPointer = NULL;
PFNGLVIEWPORTPROC glx_glViewport = NULL;
PFNGLWAITSYNCPROC glx_glWaitSync = NULL;

typedef struct { const char* name; void** slot; } glintfx_gl_sym_t;

static const glintfx_gl_sym_t glintfx_gl_symbol_table[] = {
  { "glActiveTexture", (void**)&glx_glActiveTexture },
  { "glAttachShader", (void**)&glx_glAttachShader },
  { "glBeginConditionalRender", (void**)&glx_glBeginConditionalRender },
  { "glBeginQuery", (void**)&glx_glBeginQuery },
  { "glBeginTransformFeedback", (void**)&glx_glBeginTransformFeedback },
  { "glBindAttribLocation", (void**)&glx_glBindAttribLocation },
  { "glBindBuffer", (void**)&glx_glBindBuffer },
  { "glBindBufferBase", (void**)&glx_glBindBufferBase },
  { "glBindBufferRange", (void**)&glx_glBindBufferRange },
  { "glBindFragDataLocation", (void**)&glx_glBindFragDataLocation },
  { "glBindFragDataLocationIndexed", (void**)&glx_glBindFragDataLocationIndexed },
  { "glBindFramebuffer", (void**)&glx_glBindFramebuffer },
  { "glBindRenderbuffer", (void**)&glx_glBindRenderbuffer },
  { "glBindSampler", (void**)&glx_glBindSampler },
  { "glBindTexture", (void**)&glx_glBindTexture },
  { "glBindVertexArray", (void**)&glx_glBindVertexArray },
  { "glBlendColor", (void**)&glx_glBlendColor },
  { "glBlendEquation", (void**)&glx_glBlendEquation },
  { "glBlendEquationSeparate", (void**)&glx_glBlendEquationSeparate },
  { "glBlendFunc", (void**)&glx_glBlendFunc },
  { "glBlendFuncSeparate", (void**)&glx_glBlendFuncSeparate },
  { "glBlitFramebuffer", (void**)&glx_glBlitFramebuffer },
  { "glBufferData", (void**)&glx_glBufferData },
  { "glBufferSubData", (void**)&glx_glBufferSubData },
  { "glCheckFramebufferStatus", (void**)&glx_glCheckFramebufferStatus },
  { "glClampColor", (void**)&glx_glClampColor },
  { "glClear", (void**)&glx_glClear },
  { "glClearBufferfi", (void**)&glx_glClearBufferfi },
  { "glClearBufferfv", (void**)&glx_glClearBufferfv },
  { "glClearBufferiv", (void**)&glx_glClearBufferiv },
  { "glClearBufferuiv", (void**)&glx_glClearBufferuiv },
  { "glClearColor", (void**)&glx_glClearColor },
  { "glClearDepth", (void**)&glx_glClearDepth },
  { "glClearStencil", (void**)&glx_glClearStencil },
  { "glClientWaitSync", (void**)&glx_glClientWaitSync },
  { "glColorMask", (void**)&glx_glColorMask },
  { "glColorMaski", (void**)&glx_glColorMaski },
  { "glCompileShader", (void**)&glx_glCompileShader },
  { "glCompressedTexImage1D", (void**)&glx_glCompressedTexImage1D },
  { "glCompressedTexImage2D", (void**)&glx_glCompressedTexImage2D },
  { "glCompressedTexImage3D", (void**)&glx_glCompressedTexImage3D },
  { "glCompressedTexSubImage1D", (void**)&glx_glCompressedTexSubImage1D },
  { "glCompressedTexSubImage2D", (void**)&glx_glCompressedTexSubImage2D },
  { "glCompressedTexSubImage3D", (void**)&glx_glCompressedTexSubImage3D },
  { "glCopyBufferSubData", (void**)&glx_glCopyBufferSubData },
  { "glCopyTexImage1D", (void**)&glx_glCopyTexImage1D },
  { "glCopyTexImage2D", (void**)&glx_glCopyTexImage2D },
  { "glCopyTexSubImage1D", (void**)&glx_glCopyTexSubImage1D },
  { "glCopyTexSubImage2D", (void**)&glx_glCopyTexSubImage2D },
  { "glCopyTexSubImage3D", (void**)&glx_glCopyTexSubImage3D },
  { "glCreateProgram", (void**)&glx_glCreateProgram },
  { "glCreateShader", (void**)&glx_glCreateShader },
  { "glCullFace", (void**)&glx_glCullFace },
  { "glDeleteBuffers", (void**)&glx_glDeleteBuffers },
  { "glDeleteFramebuffers", (void**)&glx_glDeleteFramebuffers },
  { "glDeleteProgram", (void**)&glx_glDeleteProgram },
  { "glDeleteQueries", (void**)&glx_glDeleteQueries },
  { "glDeleteRenderbuffers", (void**)&glx_glDeleteRenderbuffers },
  { "glDeleteSamplers", (void**)&glx_glDeleteSamplers },
  { "glDeleteShader", (void**)&glx_glDeleteShader },
  { "glDeleteSync", (void**)&glx_glDeleteSync },
  { "glDeleteTextures", (void**)&glx_glDeleteTextures },
  { "glDeleteVertexArrays", (void**)&glx_glDeleteVertexArrays },
  { "glDepthFunc", (void**)&glx_glDepthFunc },
  { "glDepthMask", (void**)&glx_glDepthMask },
  { "glDepthRange", (void**)&glx_glDepthRange },
  { "glDetachShader", (void**)&glx_glDetachShader },
  { "glDisable", (void**)&glx_glDisable },
  { "glDisableVertexAttribArray", (void**)&glx_glDisableVertexAttribArray },
  { "glDisablei", (void**)&glx_glDisablei },
  { "glDrawArrays", (void**)&glx_glDrawArrays },
  { "glDrawArraysInstanced", (void**)&glx_glDrawArraysInstanced },
  { "glDrawBuffer", (void**)&glx_glDrawBuffer },
  { "glDrawBuffers", (void**)&glx_glDrawBuffers },
  { "glDrawElements", (void**)&glx_glDrawElements },
  { "glDrawElementsBaseVertex", (void**)&glx_glDrawElementsBaseVertex },
  { "glDrawElementsInstanced", (void**)&glx_glDrawElementsInstanced },
  { "glDrawElementsInstancedBaseVertex", (void**)&glx_glDrawElementsInstancedBaseVertex },
  { "glDrawRangeElements", (void**)&glx_glDrawRangeElements },
  { "glDrawRangeElementsBaseVertex", (void**)&glx_glDrawRangeElementsBaseVertex },
  { "glEnable", (void**)&glx_glEnable },
  { "glEnableVertexAttribArray", (void**)&glx_glEnableVertexAttribArray },
  { "glEnablei", (void**)&glx_glEnablei },
  { "glEndConditionalRender", (void**)&glx_glEndConditionalRender },
  { "glEndQuery", (void**)&glx_glEndQuery },
  { "glEndTransformFeedback", (void**)&glx_glEndTransformFeedback },
  { "glFenceSync", (void**)&glx_glFenceSync },
  { "glFinish", (void**)&glx_glFinish },
  { "glFlush", (void**)&glx_glFlush },
  { "glFlushMappedBufferRange", (void**)&glx_glFlushMappedBufferRange },
  { "glFramebufferRenderbuffer", (void**)&glx_glFramebufferRenderbuffer },
  { "glFramebufferTexture", (void**)&glx_glFramebufferTexture },
  { "glFramebufferTexture1D", (void**)&glx_glFramebufferTexture1D },
  { "glFramebufferTexture2D", (void**)&glx_glFramebufferTexture2D },
  { "glFramebufferTexture3D", (void**)&glx_glFramebufferTexture3D },
  { "glFramebufferTextureLayer", (void**)&glx_glFramebufferTextureLayer },
  { "glFrontFace", (void**)&glx_glFrontFace },
  { "glGenBuffers", (void**)&glx_glGenBuffers },
  { "glGenFramebuffers", (void**)&glx_glGenFramebuffers },
  { "glGenQueries", (void**)&glx_glGenQueries },
  { "glGenRenderbuffers", (void**)&glx_glGenRenderbuffers },
  { "glGenSamplers", (void**)&glx_glGenSamplers },
  { "glGenTextures", (void**)&glx_glGenTextures },
  { "glGenVertexArrays", (void**)&glx_glGenVertexArrays },
  { "glGenerateMipmap", (void**)&glx_glGenerateMipmap },
  { "glGetActiveAttrib", (void**)&glx_glGetActiveAttrib },
  { "glGetActiveUniform", (void**)&glx_glGetActiveUniform },
  { "glGetActiveUniformBlockName", (void**)&glx_glGetActiveUniformBlockName },
  { "glGetActiveUniformBlockiv", (void**)&glx_glGetActiveUniformBlockiv },
  { "glGetActiveUniformName", (void**)&glx_glGetActiveUniformName },
  { "glGetActiveUniformsiv", (void**)&glx_glGetActiveUniformsiv },
  { "glGetAttachedShaders", (void**)&glx_glGetAttachedShaders },
  { "glGetAttribLocation", (void**)&glx_glGetAttribLocation },
  { "glGetBooleani_v", (void**)&glx_glGetBooleani_v },
  { "glGetBooleanv", (void**)&glx_glGetBooleanv },
  { "glGetBufferParameteri64v", (void**)&glx_glGetBufferParameteri64v },
  { "glGetBufferParameteriv", (void**)&glx_glGetBufferParameteriv },
  { "glGetBufferPointerv", (void**)&glx_glGetBufferPointerv },
  { "glGetBufferSubData", (void**)&glx_glGetBufferSubData },
  { "glGetCompressedTexImage", (void**)&glx_glGetCompressedTexImage },
  { "glGetDoublev", (void**)&glx_glGetDoublev },
  { "glGetError", (void**)&glx_glGetError },
  { "glGetFloatv", (void**)&glx_glGetFloatv },
  { "glGetFragDataIndex", (void**)&glx_glGetFragDataIndex },
  { "glGetFragDataLocation", (void**)&glx_glGetFragDataLocation },
  { "glGetFramebufferAttachmentParameteriv", (void**)&glx_glGetFramebufferAttachmentParameteriv },
  { "glGetInteger64i_v", (void**)&glx_glGetInteger64i_v },
  { "glGetInteger64v", (void**)&glx_glGetInteger64v },
  { "glGetIntegeri_v", (void**)&glx_glGetIntegeri_v },
  { "glGetIntegerv", (void**)&glx_glGetIntegerv },
  { "glGetMultisamplefv", (void**)&glx_glGetMultisamplefv },
  { "glGetProgramInfoLog", (void**)&glx_glGetProgramInfoLog },
  { "glGetProgramiv", (void**)&glx_glGetProgramiv },
  { "glGetQueryObjecti64v", (void**)&glx_glGetQueryObjecti64v },
  { "glGetQueryObjectiv", (void**)&glx_glGetQueryObjectiv },
  { "glGetQueryObjectui64v", (void**)&glx_glGetQueryObjectui64v },
  { "glGetQueryObjectuiv", (void**)&glx_glGetQueryObjectuiv },
  { "glGetQueryiv", (void**)&glx_glGetQueryiv },
  { "glGetRenderbufferParameteriv", (void**)&glx_glGetRenderbufferParameteriv },
  { "glGetSamplerParameterIiv", (void**)&glx_glGetSamplerParameterIiv },
  { "glGetSamplerParameterIuiv", (void**)&glx_glGetSamplerParameterIuiv },
  { "glGetSamplerParameterfv", (void**)&glx_glGetSamplerParameterfv },
  { "glGetSamplerParameteriv", (void**)&glx_glGetSamplerParameteriv },
  { "glGetShaderInfoLog", (void**)&glx_glGetShaderInfoLog },
  { "glGetShaderSource", (void**)&glx_glGetShaderSource },
  { "glGetShaderiv", (void**)&glx_glGetShaderiv },
  { "glGetString", (void**)&glx_glGetString },
  { "glGetStringi", (void**)&glx_glGetStringi },
  { "glGetSynciv", (void**)&glx_glGetSynciv },
  { "glGetTexImage", (void**)&glx_glGetTexImage },
  { "glGetTexLevelParameterfv", (void**)&glx_glGetTexLevelParameterfv },
  { "glGetTexLevelParameteriv", (void**)&glx_glGetTexLevelParameteriv },
  { "glGetTexParameterIiv", (void**)&glx_glGetTexParameterIiv },
  { "glGetTexParameterIuiv", (void**)&glx_glGetTexParameterIuiv },
  { "glGetTexParameterfv", (void**)&glx_glGetTexParameterfv },
  { "glGetTexParameteriv", (void**)&glx_glGetTexParameteriv },
  { "glGetTransformFeedbackVarying", (void**)&glx_glGetTransformFeedbackVarying },
  { "glGetUniformBlockIndex", (void**)&glx_glGetUniformBlockIndex },
  { "glGetUniformIndices", (void**)&glx_glGetUniformIndices },
  { "glGetUniformLocation", (void**)&glx_glGetUniformLocation },
  { "glGetUniformfv", (void**)&glx_glGetUniformfv },
  { "glGetUniformiv", (void**)&glx_glGetUniformiv },
  { "glGetUniformuiv", (void**)&glx_glGetUniformuiv },
  { "glGetVertexAttribIiv", (void**)&glx_glGetVertexAttribIiv },
  { "glGetVertexAttribIuiv", (void**)&glx_glGetVertexAttribIuiv },
  { "glGetVertexAttribPointerv", (void**)&glx_glGetVertexAttribPointerv },
  { "glGetVertexAttribdv", (void**)&glx_glGetVertexAttribdv },
  { "glGetVertexAttribfv", (void**)&glx_glGetVertexAttribfv },
  { "glGetVertexAttribiv", (void**)&glx_glGetVertexAttribiv },
  { "glHint", (void**)&glx_glHint },
  { "glIsBuffer", (void**)&glx_glIsBuffer },
  { "glIsEnabled", (void**)&glx_glIsEnabled },
  { "glIsEnabledi", (void**)&glx_glIsEnabledi },
  { "glIsFramebuffer", (void**)&glx_glIsFramebuffer },
  { "glIsProgram", (void**)&glx_glIsProgram },
  { "glIsQuery", (void**)&glx_glIsQuery },
  { "glIsRenderbuffer", (void**)&glx_glIsRenderbuffer },
  { "glIsSampler", (void**)&glx_glIsSampler },
  { "glIsShader", (void**)&glx_glIsShader },
  { "glIsSync", (void**)&glx_glIsSync },
  { "glIsTexture", (void**)&glx_glIsTexture },
  { "glIsVertexArray", (void**)&glx_glIsVertexArray },
  { "glLineWidth", (void**)&glx_glLineWidth },
  { "glLinkProgram", (void**)&glx_glLinkProgram },
  { "glLogicOp", (void**)&glx_glLogicOp },
  { "glMapBuffer", (void**)&glx_glMapBuffer },
  { "glMapBufferRange", (void**)&glx_glMapBufferRange },
  { "glMultiDrawArrays", (void**)&glx_glMultiDrawArrays },
  { "glMultiDrawElements", (void**)&glx_glMultiDrawElements },
  { "glMultiDrawElementsBaseVertex", (void**)&glx_glMultiDrawElementsBaseVertex },
  { "glPixelStoref", (void**)&glx_glPixelStoref },
  { "glPixelStorei", (void**)&glx_glPixelStorei },
  { "glPointParameterf", (void**)&glx_glPointParameterf },
  { "glPointParameterfv", (void**)&glx_glPointParameterfv },
  { "glPointParameteri", (void**)&glx_glPointParameteri },
  { "glPointParameteriv", (void**)&glx_glPointParameteriv },
  { "glPointSize", (void**)&glx_glPointSize },
  { "glPolygonMode", (void**)&glx_glPolygonMode },
  { "glPolygonOffset", (void**)&glx_glPolygonOffset },
  { "glPrimitiveRestartIndex", (void**)&glx_glPrimitiveRestartIndex },
  { "glProvokingVertex", (void**)&glx_glProvokingVertex },
  { "glQueryCounter", (void**)&glx_glQueryCounter },
  { "glReadBuffer", (void**)&glx_glReadBuffer },
  { "glReadPixels", (void**)&glx_glReadPixels },
  { "glRenderbufferStorage", (void**)&glx_glRenderbufferStorage },
  { "glRenderbufferStorageMultisample", (void**)&glx_glRenderbufferStorageMultisample },
  { "glSampleCoverage", (void**)&glx_glSampleCoverage },
  { "glSampleMaski", (void**)&glx_glSampleMaski },
  { "glSamplerParameterIiv", (void**)&glx_glSamplerParameterIiv },
  { "glSamplerParameterIuiv", (void**)&glx_glSamplerParameterIuiv },
  { "glSamplerParameterf", (void**)&glx_glSamplerParameterf },
  { "glSamplerParameterfv", (void**)&glx_glSamplerParameterfv },
  { "glSamplerParameteri", (void**)&glx_glSamplerParameteri },
  { "glSamplerParameteriv", (void**)&glx_glSamplerParameteriv },
  { "glScissor", (void**)&glx_glScissor },
  { "glShaderSource", (void**)&glx_glShaderSource },
  { "glStencilFunc", (void**)&glx_glStencilFunc },
  { "glStencilFuncSeparate", (void**)&glx_glStencilFuncSeparate },
  { "glStencilMask", (void**)&glx_glStencilMask },
  { "glStencilMaskSeparate", (void**)&glx_glStencilMaskSeparate },
  { "glStencilOp", (void**)&glx_glStencilOp },
  { "glStencilOpSeparate", (void**)&glx_glStencilOpSeparate },
  { "glTexBuffer", (void**)&glx_glTexBuffer },
  { "glTexImage1D", (void**)&glx_glTexImage1D },
  { "glTexImage2D", (void**)&glx_glTexImage2D },
  { "glTexImage2DMultisample", (void**)&glx_glTexImage2DMultisample },
  { "glTexImage3D", (void**)&glx_glTexImage3D },
  { "glTexImage3DMultisample", (void**)&glx_glTexImage3DMultisample },
  { "glTexParameterIiv", (void**)&glx_glTexParameterIiv },
  { "glTexParameterIuiv", (void**)&glx_glTexParameterIuiv },
  { "glTexParameterf", (void**)&glx_glTexParameterf },
  { "glTexParameterfv", (void**)&glx_glTexParameterfv },
  { "glTexParameteri", (void**)&glx_glTexParameteri },
  { "glTexParameteriv", (void**)&glx_glTexParameteriv },
  { "glTexSubImage1D", (void**)&glx_glTexSubImage1D },
  { "glTexSubImage2D", (void**)&glx_glTexSubImage2D },
  { "glTexSubImage3D", (void**)&glx_glTexSubImage3D },
  { "glTransformFeedbackVaryings", (void**)&glx_glTransformFeedbackVaryings },
  { "glUniform1f", (void**)&glx_glUniform1f },
  { "glUniform1fv", (void**)&glx_glUniform1fv },
  { "glUniform1i", (void**)&glx_glUniform1i },
  { "glUniform1iv", (void**)&glx_glUniform1iv },
  { "glUniform1ui", (void**)&glx_glUniform1ui },
  { "glUniform1uiv", (void**)&glx_glUniform1uiv },
  { "glUniform2f", (void**)&glx_glUniform2f },
  { "glUniform2fv", (void**)&glx_glUniform2fv },
  { "glUniform2i", (void**)&glx_glUniform2i },
  { "glUniform2iv", (void**)&glx_glUniform2iv },
  { "glUniform2ui", (void**)&glx_glUniform2ui },
  { "glUniform2uiv", (void**)&glx_glUniform2uiv },
  { "glUniform3f", (void**)&glx_glUniform3f },
  { "glUniform3fv", (void**)&glx_glUniform3fv },
  { "glUniform3i", (void**)&glx_glUniform3i },
  { "glUniform3iv", (void**)&glx_glUniform3iv },
  { "glUniform3ui", (void**)&glx_glUniform3ui },
  { "glUniform3uiv", (void**)&glx_glUniform3uiv },
  { "glUniform4f", (void**)&glx_glUniform4f },
  { "glUniform4fv", (void**)&glx_glUniform4fv },
  { "glUniform4i", (void**)&glx_glUniform4i },
  { "glUniform4iv", (void**)&glx_glUniform4iv },
  { "glUniform4ui", (void**)&glx_glUniform4ui },
  { "glUniform4uiv", (void**)&glx_glUniform4uiv },
  { "glUniformBlockBinding", (void**)&glx_glUniformBlockBinding },
  { "glUniformMatrix2fv", (void**)&glx_glUniformMatrix2fv },
  { "glUniformMatrix2x3fv", (void**)&glx_glUniformMatrix2x3fv },
  { "glUniformMatrix2x4fv", (void**)&glx_glUniformMatrix2x4fv },
  { "glUniformMatrix3fv", (void**)&glx_glUniformMatrix3fv },
  { "glUniformMatrix3x2fv", (void**)&glx_glUniformMatrix3x2fv },
  { "glUniformMatrix3x4fv", (void**)&glx_glUniformMatrix3x4fv },
  { "glUniformMatrix4fv", (void**)&glx_glUniformMatrix4fv },
  { "glUniformMatrix4x2fv", (void**)&glx_glUniformMatrix4x2fv },
  { "glUniformMatrix4x3fv", (void**)&glx_glUniformMatrix4x3fv },
  { "glUnmapBuffer", (void**)&glx_glUnmapBuffer },
  { "glUseProgram", (void**)&glx_glUseProgram },
  { "glValidateProgram", (void**)&glx_glValidateProgram },
  { "glVertexAttrib1d", (void**)&glx_glVertexAttrib1d },
  { "glVertexAttrib1dv", (void**)&glx_glVertexAttrib1dv },
  { "glVertexAttrib1f", (void**)&glx_glVertexAttrib1f },
  { "glVertexAttrib1fv", (void**)&glx_glVertexAttrib1fv },
  { "glVertexAttrib1s", (void**)&glx_glVertexAttrib1s },
  { "glVertexAttrib1sv", (void**)&glx_glVertexAttrib1sv },
  { "glVertexAttrib2d", (void**)&glx_glVertexAttrib2d },
  { "glVertexAttrib2dv", (void**)&glx_glVertexAttrib2dv },
  { "glVertexAttrib2f", (void**)&glx_glVertexAttrib2f },
  { "glVertexAttrib2fv", (void**)&glx_glVertexAttrib2fv },
  { "glVertexAttrib2s", (void**)&glx_glVertexAttrib2s },
  { "glVertexAttrib2sv", (void**)&glx_glVertexAttrib2sv },
  { "glVertexAttrib3d", (void**)&glx_glVertexAttrib3d },
  { "glVertexAttrib3dv", (void**)&glx_glVertexAttrib3dv },
  { "glVertexAttrib3f", (void**)&glx_glVertexAttrib3f },
  { "glVertexAttrib3fv", (void**)&glx_glVertexAttrib3fv },
  { "glVertexAttrib3s", (void**)&glx_glVertexAttrib3s },
  { "glVertexAttrib3sv", (void**)&glx_glVertexAttrib3sv },
  { "glVertexAttrib4Nbv", (void**)&glx_glVertexAttrib4Nbv },
  { "glVertexAttrib4Niv", (void**)&glx_glVertexAttrib4Niv },
  { "glVertexAttrib4Nsv", (void**)&glx_glVertexAttrib4Nsv },
  { "glVertexAttrib4Nub", (void**)&glx_glVertexAttrib4Nub },
  { "glVertexAttrib4Nubv", (void**)&glx_glVertexAttrib4Nubv },
  { "glVertexAttrib4Nuiv", (void**)&glx_glVertexAttrib4Nuiv },
  { "glVertexAttrib4Nusv", (void**)&glx_glVertexAttrib4Nusv },
  { "glVertexAttrib4bv", (void**)&glx_glVertexAttrib4bv },
  { "glVertexAttrib4d", (void**)&glx_glVertexAttrib4d },
  { "glVertexAttrib4dv", (void**)&glx_glVertexAttrib4dv },
  { "glVertexAttrib4f", (void**)&glx_glVertexAttrib4f },
  { "glVertexAttrib4fv", (void**)&glx_glVertexAttrib4fv },
  { "glVertexAttrib4iv", (void**)&glx_glVertexAttrib4iv },
  { "glVertexAttrib4s", (void**)&glx_glVertexAttrib4s },
  { "glVertexAttrib4sv", (void**)&glx_glVertexAttrib4sv },
  { "glVertexAttrib4ubv", (void**)&glx_glVertexAttrib4ubv },
  { "glVertexAttrib4uiv", (void**)&glx_glVertexAttrib4uiv },
  { "glVertexAttrib4usv", (void**)&glx_glVertexAttrib4usv },
  { "glVertexAttribDivisor", (void**)&glx_glVertexAttribDivisor },
  { "glVertexAttribI1i", (void**)&glx_glVertexAttribI1i },
  { "glVertexAttribI1iv", (void**)&glx_glVertexAttribI1iv },
  { "glVertexAttribI1ui", (void**)&glx_glVertexAttribI1ui },
  { "glVertexAttribI1uiv", (void**)&glx_glVertexAttribI1uiv },
  { "glVertexAttribI2i", (void**)&glx_glVertexAttribI2i },
  { "glVertexAttribI2iv", (void**)&glx_glVertexAttribI2iv },
  { "glVertexAttribI2ui", (void**)&glx_glVertexAttribI2ui },
  { "glVertexAttribI2uiv", (void**)&glx_glVertexAttribI2uiv },
  { "glVertexAttribI3i", (void**)&glx_glVertexAttribI3i },
  { "glVertexAttribI3iv", (void**)&glx_glVertexAttribI3iv },
  { "glVertexAttribI3ui", (void**)&glx_glVertexAttribI3ui },
  { "glVertexAttribI3uiv", (void**)&glx_glVertexAttribI3uiv },
  { "glVertexAttribI4bv", (void**)&glx_glVertexAttribI4bv },
  { "glVertexAttribI4i", (void**)&glx_glVertexAttribI4i },
  { "glVertexAttribI4iv", (void**)&glx_glVertexAttribI4iv },
  { "glVertexAttribI4sv", (void**)&glx_glVertexAttribI4sv },
  { "glVertexAttribI4ubv", (void**)&glx_glVertexAttribI4ubv },
  { "glVertexAttribI4ui", (void**)&glx_glVertexAttribI4ui },
  { "glVertexAttribI4uiv", (void**)&glx_glVertexAttribI4uiv },
  { "glVertexAttribI4usv", (void**)&glx_glVertexAttribI4usv },
  { "glVertexAttribIPointer", (void**)&glx_glVertexAttribIPointer },
  { "glVertexAttribP1ui", (void**)&glx_glVertexAttribP1ui },
  { "glVertexAttribP1uiv", (void**)&glx_glVertexAttribP1uiv },
  { "glVertexAttribP2ui", (void**)&glx_glVertexAttribP2ui },
  { "glVertexAttribP2uiv", (void**)&glx_glVertexAttribP2uiv },
  { "glVertexAttribP3ui", (void**)&glx_glVertexAttribP3ui },
  { "glVertexAttribP3uiv", (void**)&glx_glVertexAttribP3uiv },
  { "glVertexAttribP4ui", (void**)&glx_glVertexAttribP4ui },
  { "glVertexAttribP4uiv", (void**)&glx_glVertexAttribP4uiv },
  { "glVertexAttribPointer", (void**)&glx_glVertexAttribPointer },
  { "glViewport", (void**)&glx_glViewport },
  { "glWaitSync", (void**)&glx_glWaitSync },
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

#ifdef _WIN32
// EN: Win32 codepath (L1.14-GLLOADER-WIN) -- mirrors the well-known GLAD
//     GLAD_PLATFORM_WIN32 resolution strategy: wglGetProcAddress() first (covers GL
//     >=1.2 core entry points and every extension), falling back to a direct
//     GetProcAddress() against opengl32.dll (covers GL 1.0/1.1, which
//     wglGetProcAddress is documented to NEVER resolve -- see the sentinel-value
//     rationale on glintfx_resolve_symbol below). opengl32.dll is loaded via
//     LoadLibraryA(), never linked (-lopengl32) -- kernel32's implicit
//     Load/GetProcAddress import keeps this loader dependency-free for consumers'
//     link lines, matching the POSIX branch's dlopen-not-link posture.
// PT: Caminho Win32 (L1.14-GLLOADER-WIN) -- espelha a estratégia de resolução
//     GLAD_PLATFORM_WIN32 do GLAD, bem conhecida: wglGetProcAddress() primeiro
//     (cobre entry points core GL >=1.2 e toda extensão), caindo para
//     GetProcAddress() direto em opengl32.dll (cobre GL 1.0/1.1, que o
//     wglGetProcAddress é documentado a NUNCA resolver -- ver o racional dos
//     valores-sentinela em glintfx_resolve_symbol abaixo). opengl32.dll é
//     carregada via LoadLibraryA(), nunca linkada (-lopengl32) -- o import
//     implícito de Load/GetProcAddress do kernel32 mantém este loader livre de
//     dependência na linha de link dos consumidores, espelhando a postura
//     dlopen-sem-linkar do ramo POSIX.
typedef PROC (WINAPI *glintfx_wglGetProcAddress_t)(LPCSTR);

static HMODULE glintfx_opengl32 = NULL;
static glintfx_wglGetProcAddress_t glintfx_wgl_get_proc = NULL;

static void* glintfx_resolve_symbol(const char* name) {
  // EN: FARPROC/PROC are the same underlying function-pointer type on Win32
  //     (<winnt.h>/<minwindef.h>: `typedef FARPROC PROC;`), so a single union
  //     member covers both wglGetProcAddress()'s and GetProcAddress()'s return
  //     value -- same void*<->function-pointer round-trip idiom as the POSIX
  //     `cast` union below (avoids a direct object/function-pointer C-style cast).
  // PT: FARPROC/PROC são o mesmo tipo de ponteiro-de-função por baixo no Win32
  //     (<winnt.h>/<minwindef.h>: `typedef FARPROC PROC;`), então um único membro
  //     de union cobre o retorno de wglGetProcAddress() e de GetProcAddress() --
  //     mesmo idioma de round-trip void*<->ponteiro-de-função da union `cast`
  //     POSIX abaixo (evita um cast C-style direto objeto/ponteiro-de-função).
  union { void* obj; FARPROC fn; } cast;
  cast.obj = NULL;

  if (glintfx_wgl_get_proc) {
    cast.fn = glintfx_wgl_get_proc(name);
    void* p = cast.obj;
    // EN: wglGetProcAddress signals failure not only with NULL but also with the
    //     four documented sentinel values below (MSDN: "The pointer returned...
    //     may be one of a number of failure codes"). Treating those as success
    //     would hand a GL 1.0/1.1 caller a bogus non-NULL pointer that crashes on
    //     first invocation -- those entry points are ordinary opengl32.dll
    //     exports and are documented to NEVER resolve via wglGetProcAddress.
    // PT: wglGetProcAddress sinaliza falha não só com NULL, mas também com os
    //     quatro valores-sentinela documentados abaixo (MSDN: "o ponteiro
    //     retornado... pode ser um dos vários códigos de falha"). Tratar esses
    //     como sucesso entregaria a um chamador GL 1.0/1.1 um ponteiro não-NULL
    //     espúrio que crasha na primeira chamada -- esses entry points são
    //     exports comuns de opengl32.dll e são documentados a NUNCA resolver via
    //     wglGetProcAddress.
    if (p != NULL && p != (void*)1 && p != (void*)2 && p != (void*)3 && p != (void*)-1) {
      return p;
    }
  }
  if (glintfx_opengl32) {
    cast.fn = GetProcAddress(glintfx_opengl32, name);
    return cast.obj;
  }
  return NULL;
}

int glx_gl_load(void) {
  if (!glintfx_opengl32) {
    glintfx_opengl32 = LoadLibraryA("opengl32.dll");
  }

  if (glintfx_opengl32 && !glintfx_wgl_get_proc) {
    union { FARPROC obj; glintfx_wglGetProcAddress_t fn; } cast;
    cast.obj = GetProcAddress(glintfx_opengl32, "wglGetProcAddress");
    glintfx_wgl_get_proc = cast.fn;
  }

  int missing = 0;
  for (size_t i = 0; i < sizeof(glintfx_gl_symbol_table) / sizeof(glintfx_gl_symbol_table[0]); ++i) {
    void* resolved = glintfx_resolve_symbol(glintfx_gl_symbol_table[i].name);
    *glintfx_gl_symbol_table[i].slot = resolved;
    if (!resolved) ++missing;
  }
  return missing ? 1 : 0;
}
#else
typedef glintfx_glx_fn_t (*glintfx_glXGetProcAddress_t)(const unsigned char*);
typedef void* (*glintfx_eglGetProcAddress_t)(const char*);

static void* glintfx_gl_lib_handle = NULL;
static glintfx_glXGetProcAddress_t glintfx_glx_get_proc_address = NULL;
static glintfx_eglGetProcAddress_t glintfx_egl_get_proc_address = NULL;

static void* glintfx_resolve_symbol(const char* name) {
  union { void* obj; glintfx_glx_fn_t fn; } cast;
  cast.obj = NULL;

  if (glintfx_glx_get_proc_address) {
    // EN: glXGetProcAddress's ABI-mandated parameter type is `const GLubyte*` (==
    //     `const unsigned char*`), one qualifier-preserving character-type away from
    //     `name`'s `const char*`. Routed through a union member (instead of a
    //     C-style `(const unsigned char*)name` cast) for the same reason as the
    //     `cast` union above: it sidesteps cppcheck's cstyleCast style check (a
    //     project-wide CI gate, --error-exitcode=1) without a per-file suppression,
    //     and C explicitly permits inspecting any object through an unsigned-char
    //     lvalue, so this is, if anything, less exotic than the pointer-to-
    //     function-pointer union conversion two lines up.
    // PT: O tipo de parâmetro do glXGetProcAddress, ditado pela ABI, é `const
    //     GLubyte*` (== `const unsigned char*`), a um tipo-de-caractere qualificador-
    //     preservado de distância do `const char*` de `name`. Roteado por membro de
    //     union (em vez de um cast C-style `(const unsigned char*)name`) pelo mesmo
    //     motivo da union `cast` acima: contorna o check de estilo cstyleCast do
    //     cppcheck (gate de CI do projeto inteiro, --error-exitcode=1) sem
    //     supressão pontual, e o C permite explicitamente inspecionar qualquer
    //     objeto via lvalue unsigned char -- portanto isto é, no máximo, menos
    //     exótico que a conversão de ponteiro-de-objeto-para-ponteiro-de-função via
    //     union duas linhas acima.
    union { const char* c; const unsigned char* u; } name_view;
    name_view.c = name;
    cast.fn = glintfx_glx_get_proc_address(name_view.u);
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
#endif // _WIN32

#ifdef __cplusplus
}
#endif
