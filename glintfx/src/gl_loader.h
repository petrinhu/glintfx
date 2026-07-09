// SPDX-License-Identifier: MPL-2.0
// Copyright (c) 2026 Petrus Silva Costa
//
// EN: GENERATED FILE -- do not hand-edit. Produced by tools/gen_glloader.py from the
//     public Khronos gl.xml registry (Apache-2.0) + the vendored Khronos glcorearb.h
//     (MIT). See tools/gen_glloader.py for the full generation method and the
//     clean-room note (L1.14-GLLOADER: this loader was NOT written by reading the
//     previously-vendored gl3w sources).
//
//     Declares one `extern PFN<CMD>PROC <cmd>;` function-pointer variable per GL 3.3
//     core-profile command -- the variable shares the exact name of the GL function it
//     stands in for (e.g. `glCullFace`), so existing call sites (`glCullFace(x)`) keep
//     working unchanged once glx_gl_load() has populated the pointers. This is safe
//     because glcorearb.h only emits *real* `extern` function prototypes under the
//     opt-in GL_GLEXT_PROTOTYPES macro, which this header never defines.
//
//     glx_gl_load() resolves every symbol against the host's current GL context and
//     returns 0 on success (same polarity as the gl3wInit() call sites it replaces),
//     non-zero if one or more core symbols could not be resolved.
// PT: ARQUIVO GERADO -- não editar à mão. Produzido por tools/gen_glloader.py a
//     partir do registro público gl.xml da Khronos (Apache-2.0) + do glcorearb.h
//     vendorizado da Khronos (MIT). Ver tools/gen_glloader.py para o método completo
//     de geração e a nota clean-room (L1.14-GLLOADER: este loader NÃO foi escrito
//     lendo os fontes do gl3w previamente vendorizado).
//
//     Declara uma variável-ponteiro-de-função `extern PFN<CMD>PROC <cmd>;` por
//     comando do profile core GL 3.3 -- a variável tem exatamente o mesmo nome da
//     função GL que substitui (ex.: `glCullFace`), então call sites existentes
//     (`glCullFace(x)`) continuam funcionando sem mudança assim que glx_gl_load()
//     tiver preenchido os ponteiros. Isso é seguro porque glcorearb.h só emite
//     protótipos `extern` de função de verdade sob a macro opt-in
//     GL_GLEXT_PROTOTYPES, que este header nunca define.
//
//     glx_gl_load() resolve cada símbolo contra o contexto GL corrente do host e
//     retorna 0 em sucesso (mesma polaridade dos call-sites de gl3wInit() que
//     substitui), não-zero se um ou mais símbolos core não puderam ser resolvidos.
#ifndef GLINTFX_GL_LOADER_H
#define GLINTFX_GL_LOADER_H

#include <GL/glcorearb.h>

#ifdef __cplusplus
extern "C" {
#endif

extern PFNGLACTIVETEXTUREPROC glActiveTexture;
extern PFNGLATTACHSHADERPROC glAttachShader;
extern PFNGLBEGINCONDITIONALRENDERPROC glBeginConditionalRender;
extern PFNGLBEGINQUERYPROC glBeginQuery;
extern PFNGLBEGINTRANSFORMFEEDBACKPROC glBeginTransformFeedback;
extern PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBINDBUFFERBASEPROC glBindBufferBase;
extern PFNGLBINDBUFFERRANGEPROC glBindBufferRange;
extern PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation;
extern PFNGLBINDFRAGDATALOCATIONINDEXEDPROC glBindFragDataLocationIndexed;
extern PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
extern PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
extern PFNGLBINDSAMPLERPROC glBindSampler;
extern PFNGLBINDTEXTUREPROC glBindTexture;
extern PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
extern PFNGLBLENDCOLORPROC glBlendColor;
extern PFNGLBLENDEQUATIONPROC glBlendEquation;
extern PFNGLBLENDEQUATIONSEPARATEPROC glBlendEquationSeparate;
extern PFNGLBLENDFUNCPROC glBlendFunc;
extern PFNGLBLENDFUNCSEPARATEPROC glBlendFuncSeparate;
extern PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
extern PFNGLBUFFERSUBDATAPROC glBufferSubData;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
extern PFNGLCLAMPCOLORPROC glClampColor;
extern PFNGLCLEARPROC glClear;
extern PFNGLCLEARBUFFERFIPROC glClearBufferfi;
extern PFNGLCLEARBUFFERFVPROC glClearBufferfv;
extern PFNGLCLEARBUFFERIVPROC glClearBufferiv;
extern PFNGLCLEARBUFFERUIVPROC glClearBufferuiv;
extern PFNGLCLEARCOLORPROC glClearColor;
extern PFNGLCLEARDEPTHPROC glClearDepth;
extern PFNGLCLEARSTENCILPROC glClearStencil;
extern PFNGLCLIENTWAITSYNCPROC glClientWaitSync;
extern PFNGLCOLORMASKPROC glColorMask;
extern PFNGLCOLORMASKIPROC glColorMaski;
extern PFNGLCOMPILESHADERPROC glCompileShader;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC glCompressedTexImage1D;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC glCompressedTexImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC glCompressedTexImage3D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glCompressedTexSubImage1D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glCompressedTexSubImage2D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glCompressedTexSubImage3D;
extern PFNGLCOPYBUFFERSUBDATAPROC glCopyBufferSubData;
extern PFNGLCOPYTEXIMAGE1DPROC glCopyTexImage1D;
extern PFNGLCOPYTEXIMAGE2DPROC glCopyTexImage2D;
extern PFNGLCOPYTEXSUBIMAGE1DPROC glCopyTexSubImage1D;
extern PFNGLCOPYTEXSUBIMAGE2DPROC glCopyTexSubImage2D;
extern PFNGLCOPYTEXSUBIMAGE3DPROC glCopyTexSubImage3D;
extern PFNGLCREATEPROGRAMPROC glCreateProgram;
extern PFNGLCREATESHADERPROC glCreateShader;
extern PFNGLCULLFACEPROC glCullFace;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
extern PFNGLDELETEPROGRAMPROC glDeleteProgram;
extern PFNGLDELETEQUERIESPROC glDeleteQueries;
extern PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;
extern PFNGLDELETESAMPLERSPROC glDeleteSamplers;
extern PFNGLDELETESHADERPROC glDeleteShader;
extern PFNGLDELETESYNCPROC glDeleteSync;
extern PFNGLDELETETEXTURESPROC glDeleteTextures;
extern PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
extern PFNGLDEPTHFUNCPROC glDepthFunc;
extern PFNGLDEPTHMASKPROC glDepthMask;
extern PFNGLDEPTHRANGEPROC glDepthRange;
extern PFNGLDETACHSHADERPROC glDetachShader;
extern PFNGLDISABLEPROC glDisable;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
extern PFNGLDISABLEIPROC glDisablei;
extern PFNGLDRAWARRAYSPROC glDrawArrays;
extern PFNGLDRAWARRAYSINSTANCEDPROC glDrawArraysInstanced;
extern PFNGLDRAWBUFFERPROC glDrawBuffer;
extern PFNGLDRAWBUFFERSPROC glDrawBuffers;
extern PFNGLDRAWELEMENTSPROC glDrawElements;
extern PFNGLDRAWELEMENTSBASEVERTEXPROC glDrawElementsBaseVertex;
extern PFNGLDRAWELEMENTSINSTANCEDPROC glDrawElementsInstanced;
extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glDrawElementsInstancedBaseVertex;
extern PFNGLDRAWRANGEELEMENTSPROC glDrawRangeElements;
extern PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC glDrawRangeElementsBaseVertex;
extern PFNGLENABLEPROC glEnable;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
extern PFNGLENABLEIPROC glEnablei;
extern PFNGLENDCONDITIONALRENDERPROC glEndConditionalRender;
extern PFNGLENDQUERYPROC glEndQuery;
extern PFNGLENDTRANSFORMFEEDBACKPROC glEndTransformFeedback;
extern PFNGLFENCESYNCPROC glFenceSync;
extern PFNGLFINISHPROC glFinish;
extern PFNGLFLUSHPROC glFlush;
extern PFNGLFLUSHMAPPEDBUFFERRANGEPROC glFlushMappedBufferRange;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
extern PFNGLFRAMEBUFFERTEXTUREPROC glFramebufferTexture;
extern PFNGLFRAMEBUFFERTEXTURE1DPROC glFramebufferTexture1D;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
extern PFNGLFRAMEBUFFERTEXTURE3DPROC glFramebufferTexture3D;
extern PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer;
extern PFNGLFRONTFACEPROC glFrontFace;
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
extern PFNGLGENQUERIESPROC glGenQueries;
extern PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
extern PFNGLGENSAMPLERSPROC glGenSamplers;
extern PFNGLGENTEXTURESPROC glGenTextures;
extern PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
extern PFNGLGENERATEMIPMAPPROC glGenerateMipmap;
extern PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib;
extern PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
extern PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC glGetActiveUniformBlockName;
extern PFNGLGETACTIVEUNIFORMBLOCKIVPROC glGetActiveUniformBlockiv;
extern PFNGLGETACTIVEUNIFORMNAMEPROC glGetActiveUniformName;
extern PFNGLGETACTIVEUNIFORMSIVPROC glGetActiveUniformsiv;
extern PFNGLGETATTACHEDSHADERSPROC glGetAttachedShaders;
extern PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
extern PFNGLGETBOOLEANI_VPROC glGetBooleani_v;
extern PFNGLGETBOOLEANVPROC glGetBooleanv;
extern PFNGLGETBUFFERPARAMETERI64VPROC glGetBufferParameteri64v;
extern PFNGLGETBUFFERPARAMETERIVPROC glGetBufferParameteriv;
extern PFNGLGETBUFFERPOINTERVPROC glGetBufferPointerv;
extern PFNGLGETBUFFERSUBDATAPROC glGetBufferSubData;
extern PFNGLGETCOMPRESSEDTEXIMAGEPROC glGetCompressedTexImage;
extern PFNGLGETDOUBLEVPROC glGetDoublev;
extern PFNGLGETERRORPROC glGetError;
extern PFNGLGETFLOATVPROC glGetFloatv;
extern PFNGLGETFRAGDATAINDEXPROC glGetFragDataIndex;
extern PFNGLGETFRAGDATALOCATIONPROC glGetFragDataLocation;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glGetFramebufferAttachmentParameteriv;
extern PFNGLGETINTEGER64I_VPROC glGetInteger64i_v;
extern PFNGLGETINTEGER64VPROC glGetInteger64v;
extern PFNGLGETINTEGERI_VPROC glGetIntegeri_v;
extern PFNGLGETINTEGERVPROC glGetIntegerv;
extern PFNGLGETMULTISAMPLEFVPROC glGetMultisamplefv;
extern PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC glGetProgramiv;
extern PFNGLGETQUERYOBJECTI64VPROC glGetQueryObjecti64v;
extern PFNGLGETQUERYOBJECTIVPROC glGetQueryObjectiv;
extern PFNGLGETQUERYOBJECTUI64VPROC glGetQueryObjectui64v;
extern PFNGLGETQUERYOBJECTUIVPROC glGetQueryObjectuiv;
extern PFNGLGETQUERYIVPROC glGetQueryiv;
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glGetRenderbufferParameteriv;
extern PFNGLGETSAMPLERPARAMETERIIVPROC glGetSamplerParameterIiv;
extern PFNGLGETSAMPLERPARAMETERIUIVPROC glGetSamplerParameterIuiv;
extern PFNGLGETSAMPLERPARAMETERFVPROC glGetSamplerParameterfv;
extern PFNGLGETSAMPLERPARAMETERIVPROC glGetSamplerParameteriv;
extern PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
extern PFNGLGETSHADERSOURCEPROC glGetShaderSource;
extern PFNGLGETSHADERIVPROC glGetShaderiv;
extern PFNGLGETSTRINGPROC glGetString;
extern PFNGLGETSTRINGIPROC glGetStringi;
extern PFNGLGETSYNCIVPROC glGetSynciv;
extern PFNGLGETTEXIMAGEPROC glGetTexImage;
extern PFNGLGETTEXLEVELPARAMETERFVPROC glGetTexLevelParameterfv;
extern PFNGLGETTEXLEVELPARAMETERIVPROC glGetTexLevelParameteriv;
extern PFNGLGETTEXPARAMETERIIVPROC glGetTexParameterIiv;
extern PFNGLGETTEXPARAMETERIUIVPROC glGetTexParameterIuiv;
extern PFNGLGETTEXPARAMETERFVPROC glGetTexParameterfv;
extern PFNGLGETTEXPARAMETERIVPROC glGetTexParameteriv;
extern PFNGLGETTRANSFORMFEEDBACKVARYINGPROC glGetTransformFeedbackVarying;
extern PFNGLGETUNIFORMBLOCKINDEXPROC glGetUniformBlockIndex;
extern PFNGLGETUNIFORMINDICESPROC glGetUniformIndices;
extern PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
extern PFNGLGETUNIFORMFVPROC glGetUniformfv;
extern PFNGLGETUNIFORMIVPROC glGetUniformiv;
extern PFNGLGETUNIFORMUIVPROC glGetUniformuiv;
extern PFNGLGETVERTEXATTRIBIIVPROC glGetVertexAttribIiv;
extern PFNGLGETVERTEXATTRIBIUIVPROC glGetVertexAttribIuiv;
extern PFNGLGETVERTEXATTRIBPOINTERVPROC glGetVertexAttribPointerv;
extern PFNGLGETVERTEXATTRIBDVPROC glGetVertexAttribdv;
extern PFNGLGETVERTEXATTRIBFVPROC glGetVertexAttribfv;
extern PFNGLGETVERTEXATTRIBIVPROC glGetVertexAttribiv;
extern PFNGLHINTPROC glHint;
extern PFNGLISBUFFERPROC glIsBuffer;
extern PFNGLISENABLEDPROC glIsEnabled;
extern PFNGLISENABLEDIPROC glIsEnabledi;
extern PFNGLISFRAMEBUFFERPROC glIsFramebuffer;
extern PFNGLISPROGRAMPROC glIsProgram;
extern PFNGLISQUERYPROC glIsQuery;
extern PFNGLISRENDERBUFFERPROC glIsRenderbuffer;
extern PFNGLISSAMPLERPROC glIsSampler;
extern PFNGLISSHADERPROC glIsShader;
extern PFNGLISSYNCPROC glIsSync;
extern PFNGLISTEXTUREPROC glIsTexture;
extern PFNGLISVERTEXARRAYPROC glIsVertexArray;
extern PFNGLLINEWIDTHPROC glLineWidth;
extern PFNGLLINKPROGRAMPROC glLinkProgram;
extern PFNGLLOGICOPPROC glLogicOp;
extern PFNGLMAPBUFFERPROC glMapBuffer;
extern PFNGLMAPBUFFERRANGEPROC glMapBufferRange;
extern PFNGLMULTIDRAWARRAYSPROC glMultiDrawArrays;
extern PFNGLMULTIDRAWELEMENTSPROC glMultiDrawElements;
extern PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC glMultiDrawElementsBaseVertex;
extern PFNGLPIXELSTOREFPROC glPixelStoref;
extern PFNGLPIXELSTOREIPROC glPixelStorei;
extern PFNGLPOINTPARAMETERFPROC glPointParameterf;
extern PFNGLPOINTPARAMETERFVPROC glPointParameterfv;
extern PFNGLPOINTPARAMETERIPROC glPointParameteri;
extern PFNGLPOINTPARAMETERIVPROC glPointParameteriv;
extern PFNGLPOINTSIZEPROC glPointSize;
extern PFNGLPOLYGONMODEPROC glPolygonMode;
extern PFNGLPOLYGONOFFSETPROC glPolygonOffset;
extern PFNGLPRIMITIVERESTARTINDEXPROC glPrimitiveRestartIndex;
extern PFNGLPROVOKINGVERTEXPROC glProvokingVertex;
extern PFNGLQUERYCOUNTERPROC glQueryCounter;
extern PFNGLREADBUFFERPROC glReadBuffer;
extern PFNGLREADPIXELSPROC glReadPixels;
extern PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glRenderbufferStorageMultisample;
extern PFNGLSAMPLECOVERAGEPROC glSampleCoverage;
extern PFNGLSAMPLEMASKIPROC glSampleMaski;
extern PFNGLSAMPLERPARAMETERIIVPROC glSamplerParameterIiv;
extern PFNGLSAMPLERPARAMETERIUIVPROC glSamplerParameterIuiv;
extern PFNGLSAMPLERPARAMETERFPROC glSamplerParameterf;
extern PFNGLSAMPLERPARAMETERFVPROC glSamplerParameterfv;
extern PFNGLSAMPLERPARAMETERIPROC glSamplerParameteri;
extern PFNGLSAMPLERPARAMETERIVPROC glSamplerParameteriv;
extern PFNGLSCISSORPROC glScissor;
extern PFNGLSHADERSOURCEPROC glShaderSource;
extern PFNGLSTENCILFUNCPROC glStencilFunc;
extern PFNGLSTENCILFUNCSEPARATEPROC glStencilFuncSeparate;
extern PFNGLSTENCILMASKPROC glStencilMask;
extern PFNGLSTENCILMASKSEPARATEPROC glStencilMaskSeparate;
extern PFNGLSTENCILOPPROC glStencilOp;
extern PFNGLSTENCILOPSEPARATEPROC glStencilOpSeparate;
extern PFNGLTEXBUFFERPROC glTexBuffer;
extern PFNGLTEXIMAGE1DPROC glTexImage1D;
extern PFNGLTEXIMAGE2DPROC glTexImage2D;
extern PFNGLTEXIMAGE2DMULTISAMPLEPROC glTexImage2DMultisample;
extern PFNGLTEXIMAGE3DPROC glTexImage3D;
extern PFNGLTEXIMAGE3DMULTISAMPLEPROC glTexImage3DMultisample;
extern PFNGLTEXPARAMETERIIVPROC glTexParameterIiv;
extern PFNGLTEXPARAMETERIUIVPROC glTexParameterIuiv;
extern PFNGLTEXPARAMETERFPROC glTexParameterf;
extern PFNGLTEXPARAMETERFVPROC glTexParameterfv;
extern PFNGLTEXPARAMETERIPROC glTexParameteri;
extern PFNGLTEXPARAMETERIVPROC glTexParameteriv;
extern PFNGLTEXSUBIMAGE1DPROC glTexSubImage1D;
extern PFNGLTEXSUBIMAGE2DPROC glTexSubImage2D;
extern PFNGLTEXSUBIMAGE3DPROC glTexSubImage3D;
extern PFNGLTRANSFORMFEEDBACKVARYINGSPROC glTransformFeedbackVaryings;
extern PFNGLUNIFORM1FPROC glUniform1f;
extern PFNGLUNIFORM1FVPROC glUniform1fv;
extern PFNGLUNIFORM1IPROC glUniform1i;
extern PFNGLUNIFORM1IVPROC glUniform1iv;
extern PFNGLUNIFORM1UIPROC glUniform1ui;
extern PFNGLUNIFORM1UIVPROC glUniform1uiv;
extern PFNGLUNIFORM2FPROC glUniform2f;
extern PFNGLUNIFORM2FVPROC glUniform2fv;
extern PFNGLUNIFORM2IPROC glUniform2i;
extern PFNGLUNIFORM2IVPROC glUniform2iv;
extern PFNGLUNIFORM2UIPROC glUniform2ui;
extern PFNGLUNIFORM2UIVPROC glUniform2uiv;
extern PFNGLUNIFORM3FPROC glUniform3f;
extern PFNGLUNIFORM3FVPROC glUniform3fv;
extern PFNGLUNIFORM3IPROC glUniform3i;
extern PFNGLUNIFORM3IVPROC glUniform3iv;
extern PFNGLUNIFORM3UIPROC glUniform3ui;
extern PFNGLUNIFORM3UIVPROC glUniform3uiv;
extern PFNGLUNIFORM4FPROC glUniform4f;
extern PFNGLUNIFORM4FVPROC glUniform4fv;
extern PFNGLUNIFORM4IPROC glUniform4i;
extern PFNGLUNIFORM4IVPROC glUniform4iv;
extern PFNGLUNIFORM4UIPROC glUniform4ui;
extern PFNGLUNIFORM4UIVPROC glUniform4uiv;
extern PFNGLUNIFORMBLOCKBINDINGPROC glUniformBlockBinding;
extern PFNGLUNIFORMMATRIX2FVPROC glUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX2X3FVPROC glUniformMatrix2x3fv;
extern PFNGLUNIFORMMATRIX2X4FVPROC glUniformMatrix2x4fv;
extern PFNGLUNIFORMMATRIX3FVPROC glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX3X2FVPROC glUniformMatrix3x2fv;
extern PFNGLUNIFORMMATRIX3X4FVPROC glUniformMatrix3x4fv;
extern PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
extern PFNGLUNIFORMMATRIX4X2FVPROC glUniformMatrix4x2fv;
extern PFNGLUNIFORMMATRIX4X3FVPROC glUniformMatrix4x3fv;
extern PFNGLUNMAPBUFFERPROC glUnmapBuffer;
extern PFNGLUSEPROGRAMPROC glUseProgram;
extern PFNGLVALIDATEPROGRAMPROC glValidateProgram;
extern PFNGLVERTEXATTRIB1DPROC glVertexAttrib1d;
extern PFNGLVERTEXATTRIB1DVPROC glVertexAttrib1dv;
extern PFNGLVERTEXATTRIB1FPROC glVertexAttrib1f;
extern PFNGLVERTEXATTRIB1FVPROC glVertexAttrib1fv;
extern PFNGLVERTEXATTRIB1SPROC glVertexAttrib1s;
extern PFNGLVERTEXATTRIB1SVPROC glVertexAttrib1sv;
extern PFNGLVERTEXATTRIB2DPROC glVertexAttrib2d;
extern PFNGLVERTEXATTRIB2DVPROC glVertexAttrib2dv;
extern PFNGLVERTEXATTRIB2FPROC glVertexAttrib2f;
extern PFNGLVERTEXATTRIB2FVPROC glVertexAttrib2fv;
extern PFNGLVERTEXATTRIB2SPROC glVertexAttrib2s;
extern PFNGLVERTEXATTRIB2SVPROC glVertexAttrib2sv;
extern PFNGLVERTEXATTRIB3DPROC glVertexAttrib3d;
extern PFNGLVERTEXATTRIB3DVPROC glVertexAttrib3dv;
extern PFNGLVERTEXATTRIB3FPROC glVertexAttrib3f;
extern PFNGLVERTEXATTRIB3FVPROC glVertexAttrib3fv;
extern PFNGLVERTEXATTRIB3SPROC glVertexAttrib3s;
extern PFNGLVERTEXATTRIB3SVPROC glVertexAttrib3sv;
extern PFNGLVERTEXATTRIB4NBVPROC glVertexAttrib4Nbv;
extern PFNGLVERTEXATTRIB4NIVPROC glVertexAttrib4Niv;
extern PFNGLVERTEXATTRIB4NSVPROC glVertexAttrib4Nsv;
extern PFNGLVERTEXATTRIB4NUBPROC glVertexAttrib4Nub;
extern PFNGLVERTEXATTRIB4NUBVPROC glVertexAttrib4Nubv;
extern PFNGLVERTEXATTRIB4NUIVPROC glVertexAttrib4Nuiv;
extern PFNGLVERTEXATTRIB4NUSVPROC glVertexAttrib4Nusv;
extern PFNGLVERTEXATTRIB4BVPROC glVertexAttrib4bv;
extern PFNGLVERTEXATTRIB4DPROC glVertexAttrib4d;
extern PFNGLVERTEXATTRIB4DVPROC glVertexAttrib4dv;
extern PFNGLVERTEXATTRIB4FPROC glVertexAttrib4f;
extern PFNGLVERTEXATTRIB4FVPROC glVertexAttrib4fv;
extern PFNGLVERTEXATTRIB4IVPROC glVertexAttrib4iv;
extern PFNGLVERTEXATTRIB4SPROC glVertexAttrib4s;
extern PFNGLVERTEXATTRIB4SVPROC glVertexAttrib4sv;
extern PFNGLVERTEXATTRIB4UBVPROC glVertexAttrib4ubv;
extern PFNGLVERTEXATTRIB4UIVPROC glVertexAttrib4uiv;
extern PFNGLVERTEXATTRIB4USVPROC glVertexAttrib4usv;
extern PFNGLVERTEXATTRIBDIVISORPROC glVertexAttribDivisor;
extern PFNGLVERTEXATTRIBI1IPROC glVertexAttribI1i;
extern PFNGLVERTEXATTRIBI1IVPROC glVertexAttribI1iv;
extern PFNGLVERTEXATTRIBI1UIPROC glVertexAttribI1ui;
extern PFNGLVERTEXATTRIBI1UIVPROC glVertexAttribI1uiv;
extern PFNGLVERTEXATTRIBI2IPROC glVertexAttribI2i;
extern PFNGLVERTEXATTRIBI2IVPROC glVertexAttribI2iv;
extern PFNGLVERTEXATTRIBI2UIPROC glVertexAttribI2ui;
extern PFNGLVERTEXATTRIBI2UIVPROC glVertexAttribI2uiv;
extern PFNGLVERTEXATTRIBI3IPROC glVertexAttribI3i;
extern PFNGLVERTEXATTRIBI3IVPROC glVertexAttribI3iv;
extern PFNGLVERTEXATTRIBI3UIPROC glVertexAttribI3ui;
extern PFNGLVERTEXATTRIBI3UIVPROC glVertexAttribI3uiv;
extern PFNGLVERTEXATTRIBI4BVPROC glVertexAttribI4bv;
extern PFNGLVERTEXATTRIBI4IPROC glVertexAttribI4i;
extern PFNGLVERTEXATTRIBI4IVPROC glVertexAttribI4iv;
extern PFNGLVERTEXATTRIBI4SVPROC glVertexAttribI4sv;
extern PFNGLVERTEXATTRIBI4UBVPROC glVertexAttribI4ubv;
extern PFNGLVERTEXATTRIBI4UIPROC glVertexAttribI4ui;
extern PFNGLVERTEXATTRIBI4UIVPROC glVertexAttribI4uiv;
extern PFNGLVERTEXATTRIBI4USVPROC glVertexAttribI4usv;
extern PFNGLVERTEXATTRIBIPOINTERPROC glVertexAttribIPointer;
extern PFNGLVERTEXATTRIBP1UIPROC glVertexAttribP1ui;
extern PFNGLVERTEXATTRIBP1UIVPROC glVertexAttribP1uiv;
extern PFNGLVERTEXATTRIBP2UIPROC glVertexAttribP2ui;
extern PFNGLVERTEXATTRIBP2UIVPROC glVertexAttribP2uiv;
extern PFNGLVERTEXATTRIBP3UIPROC glVertexAttribP3ui;
extern PFNGLVERTEXATTRIBP3UIVPROC glVertexAttribP3uiv;
extern PFNGLVERTEXATTRIBP4UIPROC glVertexAttribP4ui;
extern PFNGLVERTEXATTRIBP4UIVPROC glVertexAttribP4uiv;
extern PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
extern PFNGLVIEWPORTPROC glViewport;
extern PFNGLWAITSYNCPROC glWaitSync;

// EN: Resolve every GL 3.3 core function pointer declared above against the host's
//     CURRENT GL context (glXMakeCurrent/eglMakeCurrent must already have been
//     called by the caller -- same contract as the gl3wInit() call sites this
//     function replaces). Safe to call more than once per process (idempotent:
//     re-resolves and overwrites the same pointers; cheap, no allocation kept
//     around).
//     Returns 0 on success, 1 if one or more core symbols failed to resolve.
// PT: Resolve todo ponteiro de função GL 3.3 core declarado acima contra o
//     contexto GL CORRENTE do host (glXMakeCurrent/eglMakeCurrent já deve ter
//     sido chamado pelo chamador -- mesmo contrato dos call-sites de gl3wInit()
//     que esta função substitui). Seguro chamar mais de uma vez por processo
//     (idempotente: re-resolve e sobrescreve os mesmos ponteiros; barato, sem
//     alocação retida).
//     Retorna 0 em sucesso, 1 se um ou mais símbolos core falharem ao resolver.
int glx_gl_load(void);

#ifdef __cplusplus
}
#endif

#endif // GLINTFX_GL_LOADER_H
