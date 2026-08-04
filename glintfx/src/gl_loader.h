// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Petrus Silva Costa
//
// EN: GENERATED FILE -- do not hand-edit. Produced by tools/gen_glloader.py from the
//     public Khronos gl.xml registry (Apache-2.0) + the vendored Khronos glcorearb.h
//     (MIT). See tools/gen_glloader.py for the full generation method and the
//     clean-room note (L1.14-GLLOADER: this loader was NOT written by reading the
//     previously-vendored gl3w sources).
//
//     Declares one `extern PFN<CMD>PROC glx_<cmd>;` function-pointer variable per GL
//     3.3 core-profile command, PREFIXED with `glx_` (AUD-L1-GLSYM, 2026-07-19) so
//     that no data symbol in libglintfx.a is ever named identically to a real GL entry
//     point -- `nm libglintfx.a` used to show a BSS `glClear`, `glClearColor`, etc.
//     (344 of them); an embed host linking its own GL calls against libglintfx.a
//     *before* libGL.so could have those references resolve to glintfx's
//     uninitialised pointer slot instead of the driver's real function, crashing on
//     first call. A second block below, AFTER both the glcorearb.h include and every
//     extern declaration, `#define`s each bare name to its glx_-prefixed variable
//     (e.g. `#define glCullFace glx_glCullFace`) purely so glintfx's OWN, INTERNAL
//     call sites (`glCullFace(x)`) keep compiling unchanged -- this header is private
//     (glintfx/src/, never installed under glintfx/include/glintfx/), so the macro
//     never reaches a consumer's translation unit. Only the typedef token
//     (`PFN<CMD>PROC`) is untouched by the macro -- the preprocessor rewrites whole
//     identifiers only, and `PFNGLCULLFACEPROC` is never equal to the `glCullFace`
//     macro name, so there is no risk of the macro clobbering the type name.
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
//     Declara uma variável-ponteiro-de-função `extern PFN<CMD>PROC glx_<cmd>;` por
//     comando do profile core GL 3.3, PREFIXADA com `glx_` (AUD-L1-GLSYM, 2026-07-19)
//     para que nenhum símbolo de dado do libglintfx.a tenha o nome exato de um entry
//     point GL real -- `nm libglintfx.a` mostrava um `glClear`, `glClearColor` etc. em
//     BSS (344 deles); um host embed que linkasse suas próprias chamadas GL contra
//     libglintfx.a ANTES de libGL.so podia ter essas referências resolvidas contra o
//     slot de ponteiro não-inicializado do glintfx em vez da função real do driver --
//     crash na primeira chamada. Um segundo bloco abaixo, DEPOIS tanto do include de
//     glcorearb.h quanto de toda declaração extern, faz `#define` de cada nome cru
//     para sua variável prefixada com glx_ (ex.: `#define glCullFace glx_glCullFace`)
//     só para que os call sites PRÓPRIOS, INTERNOS do glintfx (`glCullFace(x)`)
//     continuem compilando sem mudança -- este header é privado (glintfx/src/, nunca
//     instalado sob glintfx/include/glintfx/), então a macro nunca alcança a unidade
//     de tradução de um consumidor. Só o token do typedef (`PFN<CMD>PROC`) fica imune
//     à macro -- o pré-processador reescreve identificadores inteiros, e
//     `PFNGLCULLFACEPROC` nunca é igual ao nome de macro `glCullFace`, então não há
//     risco de a macro atropelar o nome do tipo.
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

extern PFNGLACTIVETEXTUREPROC glx_glActiveTexture;
extern PFNGLATTACHSHADERPROC glx_glAttachShader;
extern PFNGLBEGINCONDITIONALRENDERPROC glx_glBeginConditionalRender;
extern PFNGLBEGINQUERYPROC glx_glBeginQuery;
extern PFNGLBEGINTRANSFORMFEEDBACKPROC glx_glBeginTransformFeedback;
extern PFNGLBINDATTRIBLOCATIONPROC glx_glBindAttribLocation;
extern PFNGLBINDBUFFERPROC glx_glBindBuffer;
extern PFNGLBINDBUFFERBASEPROC glx_glBindBufferBase;
extern PFNGLBINDBUFFERRANGEPROC glx_glBindBufferRange;
extern PFNGLBINDFRAGDATALOCATIONPROC glx_glBindFragDataLocation;
extern PFNGLBINDFRAGDATALOCATIONINDEXEDPROC glx_glBindFragDataLocationIndexed;
extern PFNGLBINDFRAMEBUFFERPROC glx_glBindFramebuffer;
extern PFNGLBINDRENDERBUFFERPROC glx_glBindRenderbuffer;
extern PFNGLBINDSAMPLERPROC glx_glBindSampler;
extern PFNGLBINDTEXTUREPROC glx_glBindTexture;
extern PFNGLBINDVERTEXARRAYPROC glx_glBindVertexArray;
extern PFNGLBLENDCOLORPROC glx_glBlendColor;
extern PFNGLBLENDEQUATIONPROC glx_glBlendEquation;
extern PFNGLBLENDEQUATIONSEPARATEPROC glx_glBlendEquationSeparate;
extern PFNGLBLENDFUNCPROC glx_glBlendFunc;
extern PFNGLBLENDFUNCSEPARATEPROC glx_glBlendFuncSeparate;
extern PFNGLBLITFRAMEBUFFERPROC glx_glBlitFramebuffer;
extern PFNGLBUFFERDATAPROC glx_glBufferData;
extern PFNGLBUFFERSUBDATAPROC glx_glBufferSubData;
extern PFNGLCHECKFRAMEBUFFERSTATUSPROC glx_glCheckFramebufferStatus;
extern PFNGLCLAMPCOLORPROC glx_glClampColor;
extern PFNGLCLEARPROC glx_glClear;
extern PFNGLCLEARBUFFERFIPROC glx_glClearBufferfi;
extern PFNGLCLEARBUFFERFVPROC glx_glClearBufferfv;
extern PFNGLCLEARBUFFERIVPROC glx_glClearBufferiv;
extern PFNGLCLEARBUFFERUIVPROC glx_glClearBufferuiv;
extern PFNGLCLEARCOLORPROC glx_glClearColor;
extern PFNGLCLEARDEPTHPROC glx_glClearDepth;
extern PFNGLCLEARSTENCILPROC glx_glClearStencil;
extern PFNGLCLIENTWAITSYNCPROC glx_glClientWaitSync;
extern PFNGLCOLORMASKPROC glx_glColorMask;
extern PFNGLCOLORMASKIPROC glx_glColorMaski;
extern PFNGLCOMPILESHADERPROC glx_glCompileShader;
extern PFNGLCOMPRESSEDTEXIMAGE1DPROC glx_glCompressedTexImage1D;
extern PFNGLCOMPRESSEDTEXIMAGE2DPROC glx_glCompressedTexImage2D;
extern PFNGLCOMPRESSEDTEXIMAGE3DPROC glx_glCompressedTexImage3D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC glx_glCompressedTexSubImage1D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC glx_glCompressedTexSubImage2D;
extern PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC glx_glCompressedTexSubImage3D;
extern PFNGLCOPYBUFFERSUBDATAPROC glx_glCopyBufferSubData;
extern PFNGLCOPYTEXIMAGE1DPROC glx_glCopyTexImage1D;
extern PFNGLCOPYTEXIMAGE2DPROC glx_glCopyTexImage2D;
extern PFNGLCOPYTEXSUBIMAGE1DPROC glx_glCopyTexSubImage1D;
extern PFNGLCOPYTEXSUBIMAGE2DPROC glx_glCopyTexSubImage2D;
extern PFNGLCOPYTEXSUBIMAGE3DPROC glx_glCopyTexSubImage3D;
extern PFNGLCREATEPROGRAMPROC glx_glCreateProgram;
extern PFNGLCREATESHADERPROC glx_glCreateShader;
extern PFNGLCULLFACEPROC glx_glCullFace;
extern PFNGLDELETEBUFFERSPROC glx_glDeleteBuffers;
extern PFNGLDELETEFRAMEBUFFERSPROC glx_glDeleteFramebuffers;
extern PFNGLDELETEPROGRAMPROC glx_glDeleteProgram;
extern PFNGLDELETEQUERIESPROC glx_glDeleteQueries;
extern PFNGLDELETERENDERBUFFERSPROC glx_glDeleteRenderbuffers;
extern PFNGLDELETESAMPLERSPROC glx_glDeleteSamplers;
extern PFNGLDELETESHADERPROC glx_glDeleteShader;
extern PFNGLDELETESYNCPROC glx_glDeleteSync;
extern PFNGLDELETETEXTURESPROC glx_glDeleteTextures;
extern PFNGLDELETEVERTEXARRAYSPROC glx_glDeleteVertexArrays;
extern PFNGLDEPTHFUNCPROC glx_glDepthFunc;
extern PFNGLDEPTHMASKPROC glx_glDepthMask;
extern PFNGLDEPTHRANGEPROC glx_glDepthRange;
extern PFNGLDETACHSHADERPROC glx_glDetachShader;
extern PFNGLDISABLEPROC glx_glDisable;
extern PFNGLDISABLEVERTEXATTRIBARRAYPROC glx_glDisableVertexAttribArray;
extern PFNGLDISABLEIPROC glx_glDisablei;
extern PFNGLDRAWARRAYSPROC glx_glDrawArrays;
extern PFNGLDRAWARRAYSINSTANCEDPROC glx_glDrawArraysInstanced;
extern PFNGLDRAWBUFFERPROC glx_glDrawBuffer;
extern PFNGLDRAWBUFFERSPROC glx_glDrawBuffers;
extern PFNGLDRAWELEMENTSPROC glx_glDrawElements;
extern PFNGLDRAWELEMENTSBASEVERTEXPROC glx_glDrawElementsBaseVertex;
extern PFNGLDRAWELEMENTSINSTANCEDPROC glx_glDrawElementsInstanced;
extern PFNGLDRAWELEMENTSINSTANCEDBASEVERTEXPROC glx_glDrawElementsInstancedBaseVertex;
extern PFNGLDRAWRANGEELEMENTSPROC glx_glDrawRangeElements;
extern PFNGLDRAWRANGEELEMENTSBASEVERTEXPROC glx_glDrawRangeElementsBaseVertex;
extern PFNGLENABLEPROC glx_glEnable;
extern PFNGLENABLEVERTEXATTRIBARRAYPROC glx_glEnableVertexAttribArray;
extern PFNGLENABLEIPROC glx_glEnablei;
extern PFNGLENDCONDITIONALRENDERPROC glx_glEndConditionalRender;
extern PFNGLENDQUERYPROC glx_glEndQuery;
extern PFNGLENDTRANSFORMFEEDBACKPROC glx_glEndTransformFeedback;
extern PFNGLFENCESYNCPROC glx_glFenceSync;
extern PFNGLFINISHPROC glx_glFinish;
extern PFNGLFLUSHPROC glx_glFlush;
extern PFNGLFLUSHMAPPEDBUFFERRANGEPROC glx_glFlushMappedBufferRange;
extern PFNGLFRAMEBUFFERRENDERBUFFERPROC glx_glFramebufferRenderbuffer;
extern PFNGLFRAMEBUFFERTEXTUREPROC glx_glFramebufferTexture;
extern PFNGLFRAMEBUFFERTEXTURE1DPROC glx_glFramebufferTexture1D;
extern PFNGLFRAMEBUFFERTEXTURE2DPROC glx_glFramebufferTexture2D;
extern PFNGLFRAMEBUFFERTEXTURE3DPROC glx_glFramebufferTexture3D;
extern PFNGLFRAMEBUFFERTEXTURELAYERPROC glx_glFramebufferTextureLayer;
extern PFNGLFRONTFACEPROC glx_glFrontFace;
extern PFNGLGENBUFFERSPROC glx_glGenBuffers;
extern PFNGLGENFRAMEBUFFERSPROC glx_glGenFramebuffers;
extern PFNGLGENQUERIESPROC glx_glGenQueries;
extern PFNGLGENRENDERBUFFERSPROC glx_glGenRenderbuffers;
extern PFNGLGENSAMPLERSPROC glx_glGenSamplers;
extern PFNGLGENTEXTURESPROC glx_glGenTextures;
extern PFNGLGENVERTEXARRAYSPROC glx_glGenVertexArrays;
extern PFNGLGENERATEMIPMAPPROC glx_glGenerateMipmap;
extern PFNGLGETACTIVEATTRIBPROC glx_glGetActiveAttrib;
extern PFNGLGETACTIVEUNIFORMPROC glx_glGetActiveUniform;
extern PFNGLGETACTIVEUNIFORMBLOCKNAMEPROC glx_glGetActiveUniformBlockName;
extern PFNGLGETACTIVEUNIFORMBLOCKIVPROC glx_glGetActiveUniformBlockiv;
extern PFNGLGETACTIVEUNIFORMNAMEPROC glx_glGetActiveUniformName;
extern PFNGLGETACTIVEUNIFORMSIVPROC glx_glGetActiveUniformsiv;
extern PFNGLGETATTACHEDSHADERSPROC glx_glGetAttachedShaders;
extern PFNGLGETATTRIBLOCATIONPROC glx_glGetAttribLocation;
extern PFNGLGETBOOLEANI_VPROC glx_glGetBooleani_v;
extern PFNGLGETBOOLEANVPROC glx_glGetBooleanv;
extern PFNGLGETBUFFERPARAMETERI64VPROC glx_glGetBufferParameteri64v;
extern PFNGLGETBUFFERPARAMETERIVPROC glx_glGetBufferParameteriv;
extern PFNGLGETBUFFERPOINTERVPROC glx_glGetBufferPointerv;
extern PFNGLGETBUFFERSUBDATAPROC glx_glGetBufferSubData;
extern PFNGLGETCOMPRESSEDTEXIMAGEPROC glx_glGetCompressedTexImage;
extern PFNGLGETDOUBLEVPROC glx_glGetDoublev;
extern PFNGLGETERRORPROC glx_glGetError;
extern PFNGLGETFLOATVPROC glx_glGetFloatv;
extern PFNGLGETFRAGDATAINDEXPROC glx_glGetFragDataIndex;
extern PFNGLGETFRAGDATALOCATIONPROC glx_glGetFragDataLocation;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC glx_glGetFramebufferAttachmentParameteriv;
extern PFNGLGETINTEGER64I_VPROC glx_glGetInteger64i_v;
extern PFNGLGETINTEGER64VPROC glx_glGetInteger64v;
extern PFNGLGETINTEGERI_VPROC glx_glGetIntegeri_v;
extern PFNGLGETINTEGERVPROC glx_glGetIntegerv;
extern PFNGLGETMULTISAMPLEFVPROC glx_glGetMultisamplefv;
extern PFNGLGETPROGRAMINFOLOGPROC glx_glGetProgramInfoLog;
extern PFNGLGETPROGRAMIVPROC glx_glGetProgramiv;
extern PFNGLGETQUERYOBJECTI64VPROC glx_glGetQueryObjecti64v;
extern PFNGLGETQUERYOBJECTIVPROC glx_glGetQueryObjectiv;
extern PFNGLGETQUERYOBJECTUI64VPROC glx_glGetQueryObjectui64v;
extern PFNGLGETQUERYOBJECTUIVPROC glx_glGetQueryObjectuiv;
extern PFNGLGETQUERYIVPROC glx_glGetQueryiv;
extern PFNGLGETRENDERBUFFERPARAMETERIVPROC glx_glGetRenderbufferParameteriv;
extern PFNGLGETSAMPLERPARAMETERIIVPROC glx_glGetSamplerParameterIiv;
extern PFNGLGETSAMPLERPARAMETERIUIVPROC glx_glGetSamplerParameterIuiv;
extern PFNGLGETSAMPLERPARAMETERFVPROC glx_glGetSamplerParameterfv;
extern PFNGLGETSAMPLERPARAMETERIVPROC glx_glGetSamplerParameteriv;
extern PFNGLGETSHADERINFOLOGPROC glx_glGetShaderInfoLog;
extern PFNGLGETSHADERSOURCEPROC glx_glGetShaderSource;
extern PFNGLGETSHADERIVPROC glx_glGetShaderiv;
extern PFNGLGETSTRINGPROC glx_glGetString;
extern PFNGLGETSTRINGIPROC glx_glGetStringi;
extern PFNGLGETSYNCIVPROC glx_glGetSynciv;
extern PFNGLGETTEXIMAGEPROC glx_glGetTexImage;
extern PFNGLGETTEXLEVELPARAMETERFVPROC glx_glGetTexLevelParameterfv;
extern PFNGLGETTEXLEVELPARAMETERIVPROC glx_glGetTexLevelParameteriv;
extern PFNGLGETTEXPARAMETERIIVPROC glx_glGetTexParameterIiv;
extern PFNGLGETTEXPARAMETERIUIVPROC glx_glGetTexParameterIuiv;
extern PFNGLGETTEXPARAMETERFVPROC glx_glGetTexParameterfv;
extern PFNGLGETTEXPARAMETERIVPROC glx_glGetTexParameteriv;
extern PFNGLGETTRANSFORMFEEDBACKVARYINGPROC glx_glGetTransformFeedbackVarying;
extern PFNGLGETUNIFORMBLOCKINDEXPROC glx_glGetUniformBlockIndex;
extern PFNGLGETUNIFORMINDICESPROC glx_glGetUniformIndices;
extern PFNGLGETUNIFORMLOCATIONPROC glx_glGetUniformLocation;
extern PFNGLGETUNIFORMFVPROC glx_glGetUniformfv;
extern PFNGLGETUNIFORMIVPROC glx_glGetUniformiv;
extern PFNGLGETUNIFORMUIVPROC glx_glGetUniformuiv;
extern PFNGLGETVERTEXATTRIBIIVPROC glx_glGetVertexAttribIiv;
extern PFNGLGETVERTEXATTRIBIUIVPROC glx_glGetVertexAttribIuiv;
extern PFNGLGETVERTEXATTRIBPOINTERVPROC glx_glGetVertexAttribPointerv;
extern PFNGLGETVERTEXATTRIBDVPROC glx_glGetVertexAttribdv;
extern PFNGLGETVERTEXATTRIBFVPROC glx_glGetVertexAttribfv;
extern PFNGLGETVERTEXATTRIBIVPROC glx_glGetVertexAttribiv;
extern PFNGLHINTPROC glx_glHint;
extern PFNGLISBUFFERPROC glx_glIsBuffer;
extern PFNGLISENABLEDPROC glx_glIsEnabled;
extern PFNGLISENABLEDIPROC glx_glIsEnabledi;
extern PFNGLISFRAMEBUFFERPROC glx_glIsFramebuffer;
extern PFNGLISPROGRAMPROC glx_glIsProgram;
extern PFNGLISQUERYPROC glx_glIsQuery;
extern PFNGLISRENDERBUFFERPROC glx_glIsRenderbuffer;
extern PFNGLISSAMPLERPROC glx_glIsSampler;
extern PFNGLISSHADERPROC glx_glIsShader;
extern PFNGLISSYNCPROC glx_glIsSync;
extern PFNGLISTEXTUREPROC glx_glIsTexture;
extern PFNGLISVERTEXARRAYPROC glx_glIsVertexArray;
extern PFNGLLINEWIDTHPROC glx_glLineWidth;
extern PFNGLLINKPROGRAMPROC glx_glLinkProgram;
extern PFNGLLOGICOPPROC glx_glLogicOp;
extern PFNGLMAPBUFFERPROC glx_glMapBuffer;
extern PFNGLMAPBUFFERRANGEPROC glx_glMapBufferRange;
extern PFNGLMULTIDRAWARRAYSPROC glx_glMultiDrawArrays;
extern PFNGLMULTIDRAWELEMENTSPROC glx_glMultiDrawElements;
extern PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC glx_glMultiDrawElementsBaseVertex;
extern PFNGLPIXELSTOREFPROC glx_glPixelStoref;
extern PFNGLPIXELSTOREIPROC glx_glPixelStorei;
extern PFNGLPOINTPARAMETERFPROC glx_glPointParameterf;
extern PFNGLPOINTPARAMETERFVPROC glx_glPointParameterfv;
extern PFNGLPOINTPARAMETERIPROC glx_glPointParameteri;
extern PFNGLPOINTPARAMETERIVPROC glx_glPointParameteriv;
extern PFNGLPOINTSIZEPROC glx_glPointSize;
extern PFNGLPOLYGONMODEPROC glx_glPolygonMode;
extern PFNGLPOLYGONOFFSETPROC glx_glPolygonOffset;
extern PFNGLPRIMITIVERESTARTINDEXPROC glx_glPrimitiveRestartIndex;
extern PFNGLPROVOKINGVERTEXPROC glx_glProvokingVertex;
extern PFNGLQUERYCOUNTERPROC glx_glQueryCounter;
extern PFNGLREADBUFFERPROC glx_glReadBuffer;
extern PFNGLREADPIXELSPROC glx_glReadPixels;
extern PFNGLRENDERBUFFERSTORAGEPROC glx_glRenderbufferStorage;
extern PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC glx_glRenderbufferStorageMultisample;
extern PFNGLSAMPLECOVERAGEPROC glx_glSampleCoverage;
extern PFNGLSAMPLEMASKIPROC glx_glSampleMaski;
extern PFNGLSAMPLERPARAMETERIIVPROC glx_glSamplerParameterIiv;
extern PFNGLSAMPLERPARAMETERIUIVPROC glx_glSamplerParameterIuiv;
extern PFNGLSAMPLERPARAMETERFPROC glx_glSamplerParameterf;
extern PFNGLSAMPLERPARAMETERFVPROC glx_glSamplerParameterfv;
extern PFNGLSAMPLERPARAMETERIPROC glx_glSamplerParameteri;
extern PFNGLSAMPLERPARAMETERIVPROC glx_glSamplerParameteriv;
extern PFNGLSCISSORPROC glx_glScissor;
extern PFNGLSHADERSOURCEPROC glx_glShaderSource;
extern PFNGLSTENCILFUNCPROC glx_glStencilFunc;
extern PFNGLSTENCILFUNCSEPARATEPROC glx_glStencilFuncSeparate;
extern PFNGLSTENCILMASKPROC glx_glStencilMask;
extern PFNGLSTENCILMASKSEPARATEPROC glx_glStencilMaskSeparate;
extern PFNGLSTENCILOPPROC glx_glStencilOp;
extern PFNGLSTENCILOPSEPARATEPROC glx_glStencilOpSeparate;
extern PFNGLTEXBUFFERPROC glx_glTexBuffer;
extern PFNGLTEXIMAGE1DPROC glx_glTexImage1D;
extern PFNGLTEXIMAGE2DPROC glx_glTexImage2D;
extern PFNGLTEXIMAGE2DMULTISAMPLEPROC glx_glTexImage2DMultisample;
extern PFNGLTEXIMAGE3DPROC glx_glTexImage3D;
extern PFNGLTEXIMAGE3DMULTISAMPLEPROC glx_glTexImage3DMultisample;
extern PFNGLTEXPARAMETERIIVPROC glx_glTexParameterIiv;
extern PFNGLTEXPARAMETERIUIVPROC glx_glTexParameterIuiv;
extern PFNGLTEXPARAMETERFPROC glx_glTexParameterf;
extern PFNGLTEXPARAMETERFVPROC glx_glTexParameterfv;
extern PFNGLTEXPARAMETERIPROC glx_glTexParameteri;
extern PFNGLTEXPARAMETERIVPROC glx_glTexParameteriv;
extern PFNGLTEXSUBIMAGE1DPROC glx_glTexSubImage1D;
extern PFNGLTEXSUBIMAGE2DPROC glx_glTexSubImage2D;
extern PFNGLTEXSUBIMAGE3DPROC glx_glTexSubImage3D;
extern PFNGLTRANSFORMFEEDBACKVARYINGSPROC glx_glTransformFeedbackVaryings;
extern PFNGLUNIFORM1FPROC glx_glUniform1f;
extern PFNGLUNIFORM1FVPROC glx_glUniform1fv;
extern PFNGLUNIFORM1IPROC glx_glUniform1i;
extern PFNGLUNIFORM1IVPROC glx_glUniform1iv;
extern PFNGLUNIFORM1UIPROC glx_glUniform1ui;
extern PFNGLUNIFORM1UIVPROC glx_glUniform1uiv;
extern PFNGLUNIFORM2FPROC glx_glUniform2f;
extern PFNGLUNIFORM2FVPROC glx_glUniform2fv;
extern PFNGLUNIFORM2IPROC glx_glUniform2i;
extern PFNGLUNIFORM2IVPROC glx_glUniform2iv;
extern PFNGLUNIFORM2UIPROC glx_glUniform2ui;
extern PFNGLUNIFORM2UIVPROC glx_glUniform2uiv;
extern PFNGLUNIFORM3FPROC glx_glUniform3f;
extern PFNGLUNIFORM3FVPROC glx_glUniform3fv;
extern PFNGLUNIFORM3IPROC glx_glUniform3i;
extern PFNGLUNIFORM3IVPROC glx_glUniform3iv;
extern PFNGLUNIFORM3UIPROC glx_glUniform3ui;
extern PFNGLUNIFORM3UIVPROC glx_glUniform3uiv;
extern PFNGLUNIFORM4FPROC glx_glUniform4f;
extern PFNGLUNIFORM4FVPROC glx_glUniform4fv;
extern PFNGLUNIFORM4IPROC glx_glUniform4i;
extern PFNGLUNIFORM4IVPROC glx_glUniform4iv;
extern PFNGLUNIFORM4UIPROC glx_glUniform4ui;
extern PFNGLUNIFORM4UIVPROC glx_glUniform4uiv;
extern PFNGLUNIFORMBLOCKBINDINGPROC glx_glUniformBlockBinding;
extern PFNGLUNIFORMMATRIX2FVPROC glx_glUniformMatrix2fv;
extern PFNGLUNIFORMMATRIX2X3FVPROC glx_glUniformMatrix2x3fv;
extern PFNGLUNIFORMMATRIX2X4FVPROC glx_glUniformMatrix2x4fv;
extern PFNGLUNIFORMMATRIX3FVPROC glx_glUniformMatrix3fv;
extern PFNGLUNIFORMMATRIX3X2FVPROC glx_glUniformMatrix3x2fv;
extern PFNGLUNIFORMMATRIX3X4FVPROC glx_glUniformMatrix3x4fv;
extern PFNGLUNIFORMMATRIX4FVPROC glx_glUniformMatrix4fv;
extern PFNGLUNIFORMMATRIX4X2FVPROC glx_glUniformMatrix4x2fv;
extern PFNGLUNIFORMMATRIX4X3FVPROC glx_glUniformMatrix4x3fv;
extern PFNGLUNMAPBUFFERPROC glx_glUnmapBuffer;
extern PFNGLUSEPROGRAMPROC glx_glUseProgram;
extern PFNGLVALIDATEPROGRAMPROC glx_glValidateProgram;
extern PFNGLVERTEXATTRIB1DPROC glx_glVertexAttrib1d;
extern PFNGLVERTEXATTRIB1DVPROC glx_glVertexAttrib1dv;
extern PFNGLVERTEXATTRIB1FPROC glx_glVertexAttrib1f;
extern PFNGLVERTEXATTRIB1FVPROC glx_glVertexAttrib1fv;
extern PFNGLVERTEXATTRIB1SPROC glx_glVertexAttrib1s;
extern PFNGLVERTEXATTRIB1SVPROC glx_glVertexAttrib1sv;
extern PFNGLVERTEXATTRIB2DPROC glx_glVertexAttrib2d;
extern PFNGLVERTEXATTRIB2DVPROC glx_glVertexAttrib2dv;
extern PFNGLVERTEXATTRIB2FPROC glx_glVertexAttrib2f;
extern PFNGLVERTEXATTRIB2FVPROC glx_glVertexAttrib2fv;
extern PFNGLVERTEXATTRIB2SPROC glx_glVertexAttrib2s;
extern PFNGLVERTEXATTRIB2SVPROC glx_glVertexAttrib2sv;
extern PFNGLVERTEXATTRIB3DPROC glx_glVertexAttrib3d;
extern PFNGLVERTEXATTRIB3DVPROC glx_glVertexAttrib3dv;
extern PFNGLVERTEXATTRIB3FPROC glx_glVertexAttrib3f;
extern PFNGLVERTEXATTRIB3FVPROC glx_glVertexAttrib3fv;
extern PFNGLVERTEXATTRIB3SPROC glx_glVertexAttrib3s;
extern PFNGLVERTEXATTRIB3SVPROC glx_glVertexAttrib3sv;
extern PFNGLVERTEXATTRIB4NBVPROC glx_glVertexAttrib4Nbv;
extern PFNGLVERTEXATTRIB4NIVPROC glx_glVertexAttrib4Niv;
extern PFNGLVERTEXATTRIB4NSVPROC glx_glVertexAttrib4Nsv;
extern PFNGLVERTEXATTRIB4NUBPROC glx_glVertexAttrib4Nub;
extern PFNGLVERTEXATTRIB4NUBVPROC glx_glVertexAttrib4Nubv;
extern PFNGLVERTEXATTRIB4NUIVPROC glx_glVertexAttrib4Nuiv;
extern PFNGLVERTEXATTRIB4NUSVPROC glx_glVertexAttrib4Nusv;
extern PFNGLVERTEXATTRIB4BVPROC glx_glVertexAttrib4bv;
extern PFNGLVERTEXATTRIB4DPROC glx_glVertexAttrib4d;
extern PFNGLVERTEXATTRIB4DVPROC glx_glVertexAttrib4dv;
extern PFNGLVERTEXATTRIB4FPROC glx_glVertexAttrib4f;
extern PFNGLVERTEXATTRIB4FVPROC glx_glVertexAttrib4fv;
extern PFNGLVERTEXATTRIB4IVPROC glx_glVertexAttrib4iv;
extern PFNGLVERTEXATTRIB4SPROC glx_glVertexAttrib4s;
extern PFNGLVERTEXATTRIB4SVPROC glx_glVertexAttrib4sv;
extern PFNGLVERTEXATTRIB4UBVPROC glx_glVertexAttrib4ubv;
extern PFNGLVERTEXATTRIB4UIVPROC glx_glVertexAttrib4uiv;
extern PFNGLVERTEXATTRIB4USVPROC glx_glVertexAttrib4usv;
extern PFNGLVERTEXATTRIBDIVISORPROC glx_glVertexAttribDivisor;
extern PFNGLVERTEXATTRIBI1IPROC glx_glVertexAttribI1i;
extern PFNGLVERTEXATTRIBI1IVPROC glx_glVertexAttribI1iv;
extern PFNGLVERTEXATTRIBI1UIPROC glx_glVertexAttribI1ui;
extern PFNGLVERTEXATTRIBI1UIVPROC glx_glVertexAttribI1uiv;
extern PFNGLVERTEXATTRIBI2IPROC glx_glVertexAttribI2i;
extern PFNGLVERTEXATTRIBI2IVPROC glx_glVertexAttribI2iv;
extern PFNGLVERTEXATTRIBI2UIPROC glx_glVertexAttribI2ui;
extern PFNGLVERTEXATTRIBI2UIVPROC glx_glVertexAttribI2uiv;
extern PFNGLVERTEXATTRIBI3IPROC glx_glVertexAttribI3i;
extern PFNGLVERTEXATTRIBI3IVPROC glx_glVertexAttribI3iv;
extern PFNGLVERTEXATTRIBI3UIPROC glx_glVertexAttribI3ui;
extern PFNGLVERTEXATTRIBI3UIVPROC glx_glVertexAttribI3uiv;
extern PFNGLVERTEXATTRIBI4BVPROC glx_glVertexAttribI4bv;
extern PFNGLVERTEXATTRIBI4IPROC glx_glVertexAttribI4i;
extern PFNGLVERTEXATTRIBI4IVPROC glx_glVertexAttribI4iv;
extern PFNGLVERTEXATTRIBI4SVPROC glx_glVertexAttribI4sv;
extern PFNGLVERTEXATTRIBI4UBVPROC glx_glVertexAttribI4ubv;
extern PFNGLVERTEXATTRIBI4UIPROC glx_glVertexAttribI4ui;
extern PFNGLVERTEXATTRIBI4UIVPROC glx_glVertexAttribI4uiv;
extern PFNGLVERTEXATTRIBI4USVPROC glx_glVertexAttribI4usv;
extern PFNGLVERTEXATTRIBIPOINTERPROC glx_glVertexAttribIPointer;
extern PFNGLVERTEXATTRIBP1UIPROC glx_glVertexAttribP1ui;
extern PFNGLVERTEXATTRIBP1UIVPROC glx_glVertexAttribP1uiv;
extern PFNGLVERTEXATTRIBP2UIPROC glx_glVertexAttribP2ui;
extern PFNGLVERTEXATTRIBP2UIVPROC glx_glVertexAttribP2uiv;
extern PFNGLVERTEXATTRIBP3UIPROC glx_glVertexAttribP3ui;
extern PFNGLVERTEXATTRIBP3UIVPROC glx_glVertexAttribP3uiv;
extern PFNGLVERTEXATTRIBP4UIPROC glx_glVertexAttribP4ui;
extern PFNGLVERTEXATTRIBP4UIVPROC glx_glVertexAttribP4uiv;
extern PFNGLVERTEXATTRIBPOINTERPROC glx_glVertexAttribPointer;
extern PFNGLVIEWPORTPROC glx_glViewport;
extern PFNGLWAITSYNCPROC glx_glWaitSync;

// EN: Call-site-rewrite macros -- rename glintfx's OWN internal GL call sites
//     (e.g. `glCullFace(x)`) to the glx_-prefixed pointer variable declared
//     above, transparently, without touching a single .cpp call site. Private
//     header only (glintfx/src/) -- never leaks into glintfx/include/glintfx/,
//     so no consumer translation unit is ever exposed to these macros.
// PT: Macros de reescrita de call site -- renomeiam os call sites GL
//     internos, PRÓPRIOS do glintfx (ex.: `glCullFace(x)`) para a
//     variável-ponteiro prefixada com glx_ declarada acima, de forma
//     transparente, sem tocar um único call site .cpp. Só header privado
//     (glintfx/src/) -- nunca vaza para glintfx/include/glintfx/, então
//     nenhuma unidade de tradução de consumidor é exposta a estas macros.
#define glActiveTexture glx_glActiveTexture
#define glAttachShader glx_glAttachShader
#define glBeginConditionalRender glx_glBeginConditionalRender
#define glBeginQuery glx_glBeginQuery
#define glBeginTransformFeedback glx_glBeginTransformFeedback
#define glBindAttribLocation glx_glBindAttribLocation
#define glBindBuffer glx_glBindBuffer
#define glBindBufferBase glx_glBindBufferBase
#define glBindBufferRange glx_glBindBufferRange
#define glBindFragDataLocation glx_glBindFragDataLocation
#define glBindFragDataLocationIndexed glx_glBindFragDataLocationIndexed
#define glBindFramebuffer glx_glBindFramebuffer
#define glBindRenderbuffer glx_glBindRenderbuffer
#define glBindSampler glx_glBindSampler
#define glBindTexture glx_glBindTexture
#define glBindVertexArray glx_glBindVertexArray
#define glBlendColor glx_glBlendColor
#define glBlendEquation glx_glBlendEquation
#define glBlendEquationSeparate glx_glBlendEquationSeparate
#define glBlendFunc glx_glBlendFunc
#define glBlendFuncSeparate glx_glBlendFuncSeparate
#define glBlitFramebuffer glx_glBlitFramebuffer
#define glBufferData glx_glBufferData
#define glBufferSubData glx_glBufferSubData
#define glCheckFramebufferStatus glx_glCheckFramebufferStatus
#define glClampColor glx_glClampColor
#define glClear glx_glClear
#define glClearBufferfi glx_glClearBufferfi
#define glClearBufferfv glx_glClearBufferfv
#define glClearBufferiv glx_glClearBufferiv
#define glClearBufferuiv glx_glClearBufferuiv
#define glClearColor glx_glClearColor
#define glClearDepth glx_glClearDepth
#define glClearStencil glx_glClearStencil
#define glClientWaitSync glx_glClientWaitSync
#define glColorMask glx_glColorMask
#define glColorMaski glx_glColorMaski
#define glCompileShader glx_glCompileShader
#define glCompressedTexImage1D glx_glCompressedTexImage1D
#define glCompressedTexImage2D glx_glCompressedTexImage2D
#define glCompressedTexImage3D glx_glCompressedTexImage3D
#define glCompressedTexSubImage1D glx_glCompressedTexSubImage1D
#define glCompressedTexSubImage2D glx_glCompressedTexSubImage2D
#define glCompressedTexSubImage3D glx_glCompressedTexSubImage3D
#define glCopyBufferSubData glx_glCopyBufferSubData
#define glCopyTexImage1D glx_glCopyTexImage1D
#define glCopyTexImage2D glx_glCopyTexImage2D
#define glCopyTexSubImage1D glx_glCopyTexSubImage1D
#define glCopyTexSubImage2D glx_glCopyTexSubImage2D
#define glCopyTexSubImage3D glx_glCopyTexSubImage3D
#define glCreateProgram glx_glCreateProgram
#define glCreateShader glx_glCreateShader
#define glCullFace glx_glCullFace
#define glDeleteBuffers glx_glDeleteBuffers
#define glDeleteFramebuffers glx_glDeleteFramebuffers
#define glDeleteProgram glx_glDeleteProgram
#define glDeleteQueries glx_glDeleteQueries
#define glDeleteRenderbuffers glx_glDeleteRenderbuffers
#define glDeleteSamplers glx_glDeleteSamplers
#define glDeleteShader glx_glDeleteShader
#define glDeleteSync glx_glDeleteSync
#define glDeleteTextures glx_glDeleteTextures
#define glDeleteVertexArrays glx_glDeleteVertexArrays
#define glDepthFunc glx_glDepthFunc
#define glDepthMask glx_glDepthMask
#define glDepthRange glx_glDepthRange
#define glDetachShader glx_glDetachShader
#define glDisable glx_glDisable
#define glDisableVertexAttribArray glx_glDisableVertexAttribArray
#define glDisablei glx_glDisablei
#define glDrawArrays glx_glDrawArrays
#define glDrawArraysInstanced glx_glDrawArraysInstanced
#define glDrawBuffer glx_glDrawBuffer
#define glDrawBuffers glx_glDrawBuffers
#define glDrawElements glx_glDrawElements
#define glDrawElementsBaseVertex glx_glDrawElementsBaseVertex
#define glDrawElementsInstanced glx_glDrawElementsInstanced
#define glDrawElementsInstancedBaseVertex glx_glDrawElementsInstancedBaseVertex
#define glDrawRangeElements glx_glDrawRangeElements
#define glDrawRangeElementsBaseVertex glx_glDrawRangeElementsBaseVertex
#define glEnable glx_glEnable
#define glEnableVertexAttribArray glx_glEnableVertexAttribArray
#define glEnablei glx_glEnablei
#define glEndConditionalRender glx_glEndConditionalRender
#define glEndQuery glx_glEndQuery
#define glEndTransformFeedback glx_glEndTransformFeedback
#define glFenceSync glx_glFenceSync
#define glFinish glx_glFinish
#define glFlush glx_glFlush
#define glFlushMappedBufferRange glx_glFlushMappedBufferRange
#define glFramebufferRenderbuffer glx_glFramebufferRenderbuffer
#define glFramebufferTexture glx_glFramebufferTexture
#define glFramebufferTexture1D glx_glFramebufferTexture1D
#define glFramebufferTexture2D glx_glFramebufferTexture2D
#define glFramebufferTexture3D glx_glFramebufferTexture3D
#define glFramebufferTextureLayer glx_glFramebufferTextureLayer
#define glFrontFace glx_glFrontFace
#define glGenBuffers glx_glGenBuffers
#define glGenFramebuffers glx_glGenFramebuffers
#define glGenQueries glx_glGenQueries
#define glGenRenderbuffers glx_glGenRenderbuffers
#define glGenSamplers glx_glGenSamplers
#define glGenTextures glx_glGenTextures
#define glGenVertexArrays glx_glGenVertexArrays
#define glGenerateMipmap glx_glGenerateMipmap
#define glGetActiveAttrib glx_glGetActiveAttrib
#define glGetActiveUniform glx_glGetActiveUniform
#define glGetActiveUniformBlockName glx_glGetActiveUniformBlockName
#define glGetActiveUniformBlockiv glx_glGetActiveUniformBlockiv
#define glGetActiveUniformName glx_glGetActiveUniformName
#define glGetActiveUniformsiv glx_glGetActiveUniformsiv
#define glGetAttachedShaders glx_glGetAttachedShaders
#define glGetAttribLocation glx_glGetAttribLocation
#define glGetBooleani_v glx_glGetBooleani_v
#define glGetBooleanv glx_glGetBooleanv
#define glGetBufferParameteri64v glx_glGetBufferParameteri64v
#define glGetBufferParameteriv glx_glGetBufferParameteriv
#define glGetBufferPointerv glx_glGetBufferPointerv
#define glGetBufferSubData glx_glGetBufferSubData
#define glGetCompressedTexImage glx_glGetCompressedTexImage
#define glGetDoublev glx_glGetDoublev
#define glGetError glx_glGetError
#define glGetFloatv glx_glGetFloatv
#define glGetFragDataIndex glx_glGetFragDataIndex
#define glGetFragDataLocation glx_glGetFragDataLocation
#define glGetFramebufferAttachmentParameteriv glx_glGetFramebufferAttachmentParameteriv
#define glGetInteger64i_v glx_glGetInteger64i_v
#define glGetInteger64v glx_glGetInteger64v
#define glGetIntegeri_v glx_glGetIntegeri_v
#define glGetIntegerv glx_glGetIntegerv
#define glGetMultisamplefv glx_glGetMultisamplefv
#define glGetProgramInfoLog glx_glGetProgramInfoLog
#define glGetProgramiv glx_glGetProgramiv
#define glGetQueryObjecti64v glx_glGetQueryObjecti64v
#define glGetQueryObjectiv glx_glGetQueryObjectiv
#define glGetQueryObjectui64v glx_glGetQueryObjectui64v
#define glGetQueryObjectuiv glx_glGetQueryObjectuiv
#define glGetQueryiv glx_glGetQueryiv
#define glGetRenderbufferParameteriv glx_glGetRenderbufferParameteriv
#define glGetSamplerParameterIiv glx_glGetSamplerParameterIiv
#define glGetSamplerParameterIuiv glx_glGetSamplerParameterIuiv
#define glGetSamplerParameterfv glx_glGetSamplerParameterfv
#define glGetSamplerParameteriv glx_glGetSamplerParameteriv
#define glGetShaderInfoLog glx_glGetShaderInfoLog
#define glGetShaderSource glx_glGetShaderSource
#define glGetShaderiv glx_glGetShaderiv
#define glGetString glx_glGetString
#define glGetStringi glx_glGetStringi
#define glGetSynciv glx_glGetSynciv
#define glGetTexImage glx_glGetTexImage
#define glGetTexLevelParameterfv glx_glGetTexLevelParameterfv
#define glGetTexLevelParameteriv glx_glGetTexLevelParameteriv
#define glGetTexParameterIiv glx_glGetTexParameterIiv
#define glGetTexParameterIuiv glx_glGetTexParameterIuiv
#define glGetTexParameterfv glx_glGetTexParameterfv
#define glGetTexParameteriv glx_glGetTexParameteriv
#define glGetTransformFeedbackVarying glx_glGetTransformFeedbackVarying
#define glGetUniformBlockIndex glx_glGetUniformBlockIndex
#define glGetUniformIndices glx_glGetUniformIndices
#define glGetUniformLocation glx_glGetUniformLocation
#define glGetUniformfv glx_glGetUniformfv
#define glGetUniformiv glx_glGetUniformiv
#define glGetUniformuiv glx_glGetUniformuiv
#define glGetVertexAttribIiv glx_glGetVertexAttribIiv
#define glGetVertexAttribIuiv glx_glGetVertexAttribIuiv
#define glGetVertexAttribPointerv glx_glGetVertexAttribPointerv
#define glGetVertexAttribdv glx_glGetVertexAttribdv
#define glGetVertexAttribfv glx_glGetVertexAttribfv
#define glGetVertexAttribiv glx_glGetVertexAttribiv
#define glHint glx_glHint
#define glIsBuffer glx_glIsBuffer
#define glIsEnabled glx_glIsEnabled
#define glIsEnabledi glx_glIsEnabledi
#define glIsFramebuffer glx_glIsFramebuffer
#define glIsProgram glx_glIsProgram
#define glIsQuery glx_glIsQuery
#define glIsRenderbuffer glx_glIsRenderbuffer
#define glIsSampler glx_glIsSampler
#define glIsShader glx_glIsShader
#define glIsSync glx_glIsSync
#define glIsTexture glx_glIsTexture
#define glIsVertexArray glx_glIsVertexArray
#define glLineWidth glx_glLineWidth
#define glLinkProgram glx_glLinkProgram
#define glLogicOp glx_glLogicOp
#define glMapBuffer glx_glMapBuffer
#define glMapBufferRange glx_glMapBufferRange
#define glMultiDrawArrays glx_glMultiDrawArrays
#define glMultiDrawElements glx_glMultiDrawElements
#define glMultiDrawElementsBaseVertex glx_glMultiDrawElementsBaseVertex
#define glPixelStoref glx_glPixelStoref
#define glPixelStorei glx_glPixelStorei
#define glPointParameterf glx_glPointParameterf
#define glPointParameterfv glx_glPointParameterfv
#define glPointParameteri glx_glPointParameteri
#define glPointParameteriv glx_glPointParameteriv
#define glPointSize glx_glPointSize
#define glPolygonMode glx_glPolygonMode
#define glPolygonOffset glx_glPolygonOffset
#define glPrimitiveRestartIndex glx_glPrimitiveRestartIndex
#define glProvokingVertex glx_glProvokingVertex
#define glQueryCounter glx_glQueryCounter
#define glReadBuffer glx_glReadBuffer
#define glReadPixels glx_glReadPixels
#define glRenderbufferStorage glx_glRenderbufferStorage
#define glRenderbufferStorageMultisample glx_glRenderbufferStorageMultisample
#define glSampleCoverage glx_glSampleCoverage
#define glSampleMaski glx_glSampleMaski
#define glSamplerParameterIiv glx_glSamplerParameterIiv
#define glSamplerParameterIuiv glx_glSamplerParameterIuiv
#define glSamplerParameterf glx_glSamplerParameterf
#define glSamplerParameterfv glx_glSamplerParameterfv
#define glSamplerParameteri glx_glSamplerParameteri
#define glSamplerParameteriv glx_glSamplerParameteriv
#define glScissor glx_glScissor
#define glShaderSource glx_glShaderSource
#define glStencilFunc glx_glStencilFunc
#define glStencilFuncSeparate glx_glStencilFuncSeparate
#define glStencilMask glx_glStencilMask
#define glStencilMaskSeparate glx_glStencilMaskSeparate
#define glStencilOp glx_glStencilOp
#define glStencilOpSeparate glx_glStencilOpSeparate
#define glTexBuffer glx_glTexBuffer
#define glTexImage1D glx_glTexImage1D
#define glTexImage2D glx_glTexImage2D
#define glTexImage2DMultisample glx_glTexImage2DMultisample
#define glTexImage3D glx_glTexImage3D
#define glTexImage3DMultisample glx_glTexImage3DMultisample
#define glTexParameterIiv glx_glTexParameterIiv
#define glTexParameterIuiv glx_glTexParameterIuiv
#define glTexParameterf glx_glTexParameterf
#define glTexParameterfv glx_glTexParameterfv
#define glTexParameteri glx_glTexParameteri
#define glTexParameteriv glx_glTexParameteriv
#define glTexSubImage1D glx_glTexSubImage1D
#define glTexSubImage2D glx_glTexSubImage2D
#define glTexSubImage3D glx_glTexSubImage3D
#define glTransformFeedbackVaryings glx_glTransformFeedbackVaryings
#define glUniform1f glx_glUniform1f
#define glUniform1fv glx_glUniform1fv
#define glUniform1i glx_glUniform1i
#define glUniform1iv glx_glUniform1iv
#define glUniform1ui glx_glUniform1ui
#define glUniform1uiv glx_glUniform1uiv
#define glUniform2f glx_glUniform2f
#define glUniform2fv glx_glUniform2fv
#define glUniform2i glx_glUniform2i
#define glUniform2iv glx_glUniform2iv
#define glUniform2ui glx_glUniform2ui
#define glUniform2uiv glx_glUniform2uiv
#define glUniform3f glx_glUniform3f
#define glUniform3fv glx_glUniform3fv
#define glUniform3i glx_glUniform3i
#define glUniform3iv glx_glUniform3iv
#define glUniform3ui glx_glUniform3ui
#define glUniform3uiv glx_glUniform3uiv
#define glUniform4f glx_glUniform4f
#define glUniform4fv glx_glUniform4fv
#define glUniform4i glx_glUniform4i
#define glUniform4iv glx_glUniform4iv
#define glUniform4ui glx_glUniform4ui
#define glUniform4uiv glx_glUniform4uiv
#define glUniformBlockBinding glx_glUniformBlockBinding
#define glUniformMatrix2fv glx_glUniformMatrix2fv
#define glUniformMatrix2x3fv glx_glUniformMatrix2x3fv
#define glUniformMatrix2x4fv glx_glUniformMatrix2x4fv
#define glUniformMatrix3fv glx_glUniformMatrix3fv
#define glUniformMatrix3x2fv glx_glUniformMatrix3x2fv
#define glUniformMatrix3x4fv glx_glUniformMatrix3x4fv
#define glUniformMatrix4fv glx_glUniformMatrix4fv
#define glUniformMatrix4x2fv glx_glUniformMatrix4x2fv
#define glUniformMatrix4x3fv glx_glUniformMatrix4x3fv
#define glUnmapBuffer glx_glUnmapBuffer
#define glUseProgram glx_glUseProgram
#define glValidateProgram glx_glValidateProgram
#define glVertexAttrib1d glx_glVertexAttrib1d
#define glVertexAttrib1dv glx_glVertexAttrib1dv
#define glVertexAttrib1f glx_glVertexAttrib1f
#define glVertexAttrib1fv glx_glVertexAttrib1fv
#define glVertexAttrib1s glx_glVertexAttrib1s
#define glVertexAttrib1sv glx_glVertexAttrib1sv
#define glVertexAttrib2d glx_glVertexAttrib2d
#define glVertexAttrib2dv glx_glVertexAttrib2dv
#define glVertexAttrib2f glx_glVertexAttrib2f
#define glVertexAttrib2fv glx_glVertexAttrib2fv
#define glVertexAttrib2s glx_glVertexAttrib2s
#define glVertexAttrib2sv glx_glVertexAttrib2sv
#define glVertexAttrib3d glx_glVertexAttrib3d
#define glVertexAttrib3dv glx_glVertexAttrib3dv
#define glVertexAttrib3f glx_glVertexAttrib3f
#define glVertexAttrib3fv glx_glVertexAttrib3fv
#define glVertexAttrib3s glx_glVertexAttrib3s
#define glVertexAttrib3sv glx_glVertexAttrib3sv
#define glVertexAttrib4Nbv glx_glVertexAttrib4Nbv
#define glVertexAttrib4Niv glx_glVertexAttrib4Niv
#define glVertexAttrib4Nsv glx_glVertexAttrib4Nsv
#define glVertexAttrib4Nub glx_glVertexAttrib4Nub
#define glVertexAttrib4Nubv glx_glVertexAttrib4Nubv
#define glVertexAttrib4Nuiv glx_glVertexAttrib4Nuiv
#define glVertexAttrib4Nusv glx_glVertexAttrib4Nusv
#define glVertexAttrib4bv glx_glVertexAttrib4bv
#define glVertexAttrib4d glx_glVertexAttrib4d
#define glVertexAttrib4dv glx_glVertexAttrib4dv
#define glVertexAttrib4f glx_glVertexAttrib4f
#define glVertexAttrib4fv glx_glVertexAttrib4fv
#define glVertexAttrib4iv glx_glVertexAttrib4iv
#define glVertexAttrib4s glx_glVertexAttrib4s
#define glVertexAttrib4sv glx_glVertexAttrib4sv
#define glVertexAttrib4ubv glx_glVertexAttrib4ubv
#define glVertexAttrib4uiv glx_glVertexAttrib4uiv
#define glVertexAttrib4usv glx_glVertexAttrib4usv
#define glVertexAttribDivisor glx_glVertexAttribDivisor
#define glVertexAttribI1i glx_glVertexAttribI1i
#define glVertexAttribI1iv glx_glVertexAttribI1iv
#define glVertexAttribI1ui glx_glVertexAttribI1ui
#define glVertexAttribI1uiv glx_glVertexAttribI1uiv
#define glVertexAttribI2i glx_glVertexAttribI2i
#define glVertexAttribI2iv glx_glVertexAttribI2iv
#define glVertexAttribI2ui glx_glVertexAttribI2ui
#define glVertexAttribI2uiv glx_glVertexAttribI2uiv
#define glVertexAttribI3i glx_glVertexAttribI3i
#define glVertexAttribI3iv glx_glVertexAttribI3iv
#define glVertexAttribI3ui glx_glVertexAttribI3ui
#define glVertexAttribI3uiv glx_glVertexAttribI3uiv
#define glVertexAttribI4bv glx_glVertexAttribI4bv
#define glVertexAttribI4i glx_glVertexAttribI4i
#define glVertexAttribI4iv glx_glVertexAttribI4iv
#define glVertexAttribI4sv glx_glVertexAttribI4sv
#define glVertexAttribI4ubv glx_glVertexAttribI4ubv
#define glVertexAttribI4ui glx_glVertexAttribI4ui
#define glVertexAttribI4uiv glx_glVertexAttribI4uiv
#define glVertexAttribI4usv glx_glVertexAttribI4usv
#define glVertexAttribIPointer glx_glVertexAttribIPointer
#define glVertexAttribP1ui glx_glVertexAttribP1ui
#define glVertexAttribP1uiv glx_glVertexAttribP1uiv
#define glVertexAttribP2ui glx_glVertexAttribP2ui
#define glVertexAttribP2uiv glx_glVertexAttribP2uiv
#define glVertexAttribP3ui glx_glVertexAttribP3ui
#define glVertexAttribP3uiv glx_glVertexAttribP3uiv
#define glVertexAttribP4ui glx_glVertexAttribP4ui
#define glVertexAttribP4uiv glx_glVertexAttribP4uiv
#define glVertexAttribPointer glx_glVertexAttribPointer
#define glViewport glx_glViewport
#define glWaitSync glx_glWaitSync

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

// EN: `GLLOADER-HOST` (2026-08-04) -- resolve every GL 3.3 core function pointer declared
//     above by delegating EACH symbol lookup to a HOST-supplied resolver, instead of this
//     loader's own glX/EGL/dlsym chain (`glx_gl_load()` above). For a host whose GL context
//     comes from a loader glintfx's own three internal paths do not cover -- SDL
//     (`SDL_GL_GetProcAddress`), ANGLE, a host's own EGL wrapper, an engine that already
//     embeds glad/GLEW -- this is the entry point that lets the host's OWN resolver populate
//     glintfx's `glx_`-prefixed table (see `UiLayerConfig::gl_proc_resolver`,
//     `glintfx/include/glintfx/ui_layer.hpp`, for the public C++ contract this backs; this C
//     entry point is the mechanism, that header is the source of truth for policy).
//     EXCLUSIVE, no fallback: every symbol is resolved by calling `resolver(name)` ONCE, and
//     ONLY that result is stored -- never falling through to glX/EGL/dlsym on a NULL result.
//     Falling back would mean silently mixing two different GL contexts' function pointers
//     into the same table whenever a host's resolver legitimately returns NULL for a symbol
//     its context does not support -- exactly the kind of masked-context bug this entry point
//     exists to avoid, not reproduce.
//     `resolver` MUST NOT be NULL (the caller -- `ui_layer.cpp`'s constructor -- only reaches
//     this function when `Config::gl_proc_resolver` is non-null; this function does not
//     itself re-check, matching `glx_gl_load()`'s own no-redundant-guard style for an
//     already-validated internal call path).
//     Returns 0 on success, 1 if one or more core symbols could not be resolved -- SAME
//     polarity and SAME missing-symbol-count contract as `glx_gl_load()` above; on failure,
//     writes ONE diagnostic line to stderr naming the FIRST unresolved symbol and the total
//     count, tagged with the "host resolver" origin (this loader has no C++ logging funnel to
//     route through -- see `gl_loader.c`'s own header comment -- so this is the same
//     stdio-direct posture some of `glx_gl_load()`'s own callers already use, e.g.
//     `draw2d.cpp`'s `glx_gl_load() != 0` branch). Platform-independent -- unlike
//     `glx_gl_load()`, this entry point never touches glX/EGL/wgl/dlopen itself, so there is a
//     SINGLE implementation shared by every platform (see `gl_loader.c`).
// PT: `GLLOADER-HOST` (2026-08-04) -- resolve todo ponteiro de função GL 3.3 core declarado
//     acima delegando CADA busca de símbolo a um resolvedor FORNECIDO PELO HOST, em vez da
//     própria cadeia glX/EGL/dlsym deste loader (`glx_gl_load()` acima). Para um host cujo
//     contexto GL vem de um loader que os três caminhos internos da glintfx não cobrem -- SDL
//     (`SDL_GL_GetProcAddress`), ANGLE, um wrapper EGL próprio do host, um motor que já
//     embarca glad/GLEW -- este é o ponto de entrada que deixa o PRÓPRIO resolvedor do host
//     popular a tabela prefixada com `glx_` da glintfx (ver `UiLayerConfig::gl_proc_resolver`,
//     `glintfx/include/glintfx/ui_layer.hpp`, pro contrato público C++ que isto sustenta; este
//     ponto de entrada C é o mecanismo, aquele header é a fonte de verdade da política).
//     EXCLUSIVO, sem fallback: cada símbolo é resolvido chamando `resolver(name)` UMA vez, e
//     SÓ esse resultado é guardado -- nunca cai para glX/EGL/dlsym num resultado NULO. Cair
//     de volta significaria misturar em silêncio ponteiros de função de DOIS contextos GL
//     diferentes na mesma tabela sempre que o resolvedor de um host legitimamente devolvesse
//     NULL pra um símbolo que o contexto dele não suporta -- exatamente a classe de bug de
//     contexto mascarado que este ponto de entrada existe pra evitar, não reproduzir.
//     `resolver` NÃO PODE ser NULL (o chamador -- o construtor de `ui_layer.cpp` -- só
//     alcança esta função quando `Config::gl_proc_resolver` é não-nulo; esta função não
//     recheca por conta própria, no mesmo estilo sem-guarda-redundante de `glx_gl_load()` pra
//     um caminho de chamada interno já validado).
//     Retorna 0 em sucesso, 1 se um ou mais símbolos core não puderam ser resolvidos -- MESMA
//     polaridade e MESMO contrato de contagem de símbolos ausentes que `glx_gl_load()` acima;
//     em falha, escreve UMA linha de diagnóstico em stderr nomeando o PRIMEIRO símbolo não
//     resolvido e a contagem total, marcada com a origem "host resolver" (este loader não tem
//     funil de log C++ pra rotear -- ver o próprio comentário de cabeçalho de `gl_loader.c` --
//     então esta é a mesma postura stdio-direto que alguns chamadores do próprio
//     `glx_gl_load()` já usam, ex.: o ramo `glx_gl_load() != 0` de `draw2d.cpp`).
//     Independente de plataforma -- diferente do `glx_gl_load()`, este ponto de entrada nunca
//     toca glX/EGL/wgl/dlopen por conta própria, então há UMA ÚNICA implementação
//     compartilhada por toda plataforma (ver `gl_loader.c`).
int glx_gl_load_with(void* (*resolver)(const char* name));

// EN: DOC-GLCOHAB (framework-2D) -- resolve exactly ONE GL/GLX/EGL/WGL function pointer by
//     name, for a HOST that cohabits glintfx::App's GL context (set_frame_callback,
//     app.hpp) and needs an SDL_GL_GetProcAddress-equivalent without linking SDL, GLFW, or
//     any other loader itself. The sole public entry point into this file's loader: every
//     other symbol here (glintfx_resolve_symbol, the glintfx_gl_symbol_table, the glX/EGL/
//     dlopen state) stays internal-linkage/local to this translation unit, so this call
//     cannot be used to reach anything but the ONE pointer it returns -- see this file's own
//     header comment for why the batch glx_gl_load() entry point above is deliberately NOT
//     the public surface for this use case (it would pull in and expose the whole GL 3.3 core
//     symbol table this loader privately owns, ADR-0013's own glx_-prefix boundary). Delegates
//     to the EXACT SAME glintfx_resolve_symbol() static function glx_gl_load() itself calls
//     per table entry (gl_loader.c) -- zero duplicated resolution logic, glX->EGL->dlsym on
//     POSIX / wglGetProcAddress->GetProcAddress on Win32, both already-battle-tested paths.
//     NULL-safe for the caller's OWN input: `name == NULL || name[0] == '\0'` returns NULL
//     without touching the resolver; a call made BEFORE glx_gl_load() has run at least once in
//     this process also returns NULL (the underlying glX/EGL/dlopen/wgl handles
//     glintfx_resolve_symbol() reads are only populated there, so every branch falls through).
//     ⚠ NOT NULL-safe for a name that names no real symbol: measured empirically under this
//     library's own Mesa/llvmpipe CI environment, glintfx_resolve_symbol() can return a
//     NON-NULL pointer for a made-up name -- this is GLX_ARB_get_proc_address's own documented
//     behaviour (an implementation MAY hand back a non-NULL pointer for a name it does not
//     actually recognise; the caller is expected to confirm the extension/function exists
//     BEFORE calling the returned pointer), not a bug in this resolver. Only ever pass a name
//     you already know should exist for the current GL version/extension set -- same caveat
//     that applies to calling glXGetProcAddressARB/SDL_GL_GetProcAddress directly.
// PT: DOC-GLCOHAB (framework-2D) -- resolve exatamente UM ponteiro de função GL/GLX/EGL/WGL
//     por nome, para um HOST que coabita o contexto GL do glintfx::App (set_frame_callback,
//     app.hpp) e precisa de um equivalente a SDL_GL_GetProcAddress sem linkar SDL, GLFW, ou
//     qualquer outro loader por conta própria. O único ponto de entrada público neste loader:
//     todo outro símbolo aqui (glintfx_resolve_symbol, a glintfx_gl_symbol_table, o estado
//     glX/EGL/dlopen) permanece de linkagem interna/local a esta unidade de tradução, então
//     esta chamada não pode ser usada para alcançar nada além do ÚNICO ponteiro que devolve --
//     ver o próprio comentário de cabeçalho deste arquivo pro motivo do ponto de entrada em
//     lote glx_gl_load() acima ser deliberadamente NÃO a superfície pública pra este caso de
//     uso (puxaria e exporia a tabela de símbolos GL 3.3 core inteira que este loader possui em
//     privado, a própria fronteira de prefixo glx_ da ADR-0013). Delega pro MESMO
//     glintfx_resolve_symbol() estático exato que o próprio glx_gl_load() chama por entrada de
//     tabela (gl_loader.c) -- zero lógica de resolução duplicada, glX->EGL->dlsym no POSIX /
//     wglGetProcAddress->GetProcAddress no Win32, os dois caminhos já testados em batalha.
//     Seguro a NULL pra entrada PRÓPRIA do chamador: `name == NULL || name[0] == '\0'` retorna
//     NULL sem tocar o resolver; uma chamada feita ANTES do glx_gl_load() ter rodado ao menos
//     uma vez neste processo também retorna NULL (os handles glX/EGL/dlopen/wgl subjacentes que
//     glintfx_resolve_symbol() lê só são populados lá, então todo ramo cai por falta de handle).
//     ⚠ NÃO é seguro a NULL pra um nome que não nomeia símbolo nenhum: medido empiricamente sob
//     o próprio ambiente de CI Mesa/llvmpipe desta biblioteca, o glintfx_resolve_symbol() pode
//     retornar um ponteiro NÃO-NULO pra um nome inventado -- este é o próprio comportamento
//     documentado do GLX_ARB_get_proc_address (uma implementação PODE devolver um ponteiro
//     não-nulo pra um nome que não reconhece de fato; o chamador é esperado a confirmar que a
//     extensão/função existe ANTES de chamar o ponteiro devolvido), não um bug deste resolver.
//     Só passe um nome que você já sabe que deveria existir pro conjunto corrente de
//     versão/extensão GL -- a mesma ressalva que se aplica a chamar
//     glXGetProcAddressARB/SDL_GL_GetProcAddress diretamente.
//
// EN: ⚠ BUILD-PRESENCE CORRECTION (`DOC-GLPROC-CLAIM`, 2026-07-29 review finding on
//     `f0d5d88`, which claimed via `nm` that "no symbol enters the embed-only .a" --
//     true for the PUBLIC C++ symbol below, false for THIS one). This C function is declared
//     and DEFINED in EVERY build configuration, including embed-only
//     (`GLINTFX_BACKEND_GLFW=OFF`): `gl_loader.c` is the sole source of the `glloader`
//     INTERFACE library, compiled unconditionally into every consuming target
//     (`glintfx/CMakeLists.txt`, the `glloader` target definition -- no `#ifdef` module gate
//     exists anywhere in `gl_loader.c`, not even around `glx_gl_load()` itself). `nm
//     libglintfx.a` on an embed-only archive therefore DOES show `glx_gl_get_proc_address` as
//     a global `T` (function) symbol -- confirmed empirically. This is NOT a contract
//     violation: [ADR-0013](../../docs/adr/0013-gl-symbol-boundary.md) promises absence of
//     COLLISION with a host's own symbols ("must not collide"), not absence of the symbol
//     itself -- the `glx_` prefix IS that mitigation, and it holds here exactly as it does for
//     every other `glx_*` name this file exports. What IS App-mode-only and genuinely absent
//     from an embed-only `.a` is the PUBLIC C++ free function this helper backs,
//     `glintfx::gl_proc_address()` (`glintfx/include/glintfx/gl_proc.hpp`,
//     `glintfx/src/gl_proc.cpp`) -- that translation unit is gated on `GLINTFX_MODULE_APP`
//     (`glintfx/CMakeLists.txt:834-870`) and simply never compiles in an embed-only build. Do
//     not conflate the two when reasoning about what an embed host links.
// PT: ⚠ CORREÇÃO DE PRESENÇA-NO-BUILD (`DOC-GLPROC-CLAIM`, achado de review de 2026-07-29 no
//     `f0d5d88`, que afirmou via `nm` que "nenhum símbolo entra no `.a` embed-only" -- verdade
//     pro símbolo C++ PÚBLICO abaixo, falso pra ESTE aqui). Esta função C é declarada e
//     DEFINIDA em TODA configuração de build, inclusive embed-only
//     (`GLINTFX_BACKEND_GLFW=OFF`): `gl_loader.c` é a única fonte da biblioteca INTERFACE
//     `glloader`, compilada incondicionalmente em todo target consumidor
//     (`glintfx/CMakeLists.txt`, a definição do target `glloader` -- não existe `#ifdef` de
//     gate de módulo em lugar nenhum de `gl_loader.c`, nem mesmo em volta do próprio
//     `glx_gl_load()`). `nm libglintfx.a` num archive embed-only portanto MOSTRA
//     `glx_gl_get_proc_address` como símbolo global `T` (função) -- confirmado
//     empiricamente. Isto NÃO é violação de contrato: a
//     [ADR-0013](../../docs/adr/0013-gl-symbol-boundary.md) promete ausência de COLISÃO com os
//     símbolos do próprio host ("must not collide"), não ausência do símbolo em si -- o
//     prefixo `glx_` É essa mitigação, e vale aqui exatamente como vale pra todo outro nome
//     `glx_*` que este arquivo exporta. O que É só-modo-App e genuinamente ausente de um `.a`
//     embed-only é a função livre C++ PÚBLICA que este helper sustenta,
//     `glintfx::gl_proc_address()` (`glintfx/include/glintfx/gl_proc.hpp`,
//     `glintfx/src/gl_proc.cpp`) -- aquela unidade de tradução é gateada em
//     `GLINTFX_MODULE_APP` (`glintfx/CMakeLists.txt:834-870`) e simplesmente nunca compila num
//     build embed-only. Não confunda as duas ao raciocinar sobre o que um host embed linka.
void* glx_gl_get_proc_address(const char* name);

#ifdef __cplusplus
}
#endif

#endif // GLINTFX_GL_LOADER_H
