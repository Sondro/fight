OpenGLProc(USEPROGRAM, UseProgram)

#ifndef BUILD_LINUX
OpenGLProc(ACTIVETEXTURE, ActiveTexture)
#endif
OpenGLProc(GENERATEMIPMAP, GenerateMipmap)

OpenGLProc(GENFRAMEBUFFERS, GenFramebuffers)
OpenGLProc(DELETEFRAMEBUFFERS, DeleteFramebuffers)
OpenGLProc(BINDFRAMEBUFFER, BindFramebuffer)
OpenGLProc(FRAMEBUFFERTEXTURE, FramebufferTexture)
OpenGLProc(FRAMEBUFFERTEXTURE2D, FramebufferTexture2D)

OpenGLProc(GENBUFFERS, GenBuffers)
OpenGLProc(DELETEBUFFERS, DeleteBuffers)
OpenGLProc(BINDBUFFER, BindBuffer)
OpenGLProc(DRAWBUFFERS, DrawBuffers)
OpenGLProc(DRAWARRAYSINSTANCED, DrawArraysInstanced)

OpenGLProc(GENVERTEXARRAYS, GenVertexArrays)
OpenGLProc(DELETEVERTEXARRAYS, DeleteVertexArrays)
OpenGLProc(BUFFERDATA, BufferData)
OpenGLProc(BUFFERSUBDATA, BufferSubData)
OpenGLProc(BINDVERTEXARRAY, BindVertexArray)
OpenGLProc(ENABLEVERTEXATTRIBARRAY, EnableVertexAttribArray)
OpenGLProc(DISABLEVERTEXATTRIBARRAY, DisableVertexAttribArray)
OpenGLProc(VERTEXATTRIBPOINTER, VertexAttribPointer)
OpenGLProc(VERTEXATTRIBIPOINTER, VertexAttribIPointer)
OpenGLProc(VERTEXATTRIBDIVISOR, VertexAttribDivisor)

OpenGLProc(GETUNIFORMLOCATION, GetUniformLocation)
OpenGLProc(UNIFORMMATRIX4FV, UniformMatrix4fv)
OpenGLProc(UNIFORM1I, Uniform1i)
OpenGLProc(UNIFORM1F, Uniform1f)
OpenGLProc(UNIFORM2F, Uniform2f)
OpenGLProc(UNIFORM3F, Uniform3f)
OpenGLProc(UNIFORM4F, Uniform4f)

OpenGLProc(CREATESHADER, CreateShader)
OpenGLProc(SHADERSOURCE, ShaderSource)
OpenGLProc(COMPILESHADER, CompileShader)
OpenGLProc(GETSHADERIV, GetShaderiv)
OpenGLProc(ATTACHSHADER, AttachShader)
OpenGLProc(GETSHADERINFOLOG, GetShaderInfoLog)
OpenGLProc(CREATEPROGRAM, CreateProgram)
OpenGLProc(BINDATTRIBLOCATION, BindAttribLocation)
OpenGLProc(BINDFRAGDATALOCATION, BindFragDataLocation)
OpenGLProc(DELETEPROGRAM, DeleteProgram)
OpenGLProc(LINKPROGRAM, LinkProgram)
OpenGLProc(DELETESHADER, DeleteShader)

OpenGLProc(BLENDFUNCSEPARATE, BlendFuncSeparate)

#undef OpenGLProc