var gl = null;
var yuvCanvas = null;
var vertexShaderSource = [
        "attribute highp vec4 aVertexPosition;",
        "attribute vec2 aTextureCoord;",
        "varying highp vec2 vTextureCoord;",
        "void main(void) {",
        " gl_Position = aVertexPosition;",
        " vTextureCoord = aTextureCoord;",
        "}"
    ].join("\n");
var fragmentShaderSource = [
        "precision highp float;",
        "varying lowp vec2 vTextureCoord;",
        "uniform sampler2D YTexture;",
        "uniform sampler2D UTexture;",
        "uniform sampler2D VTexture;",
        "const mat4 YUV2RGB = mat4",
        "(",
        " 1.1643828125, 0, 1.59602734375, -.87078515625,",
        " 1.1643828125, -.39176171875, -.81296875, .52959375,",
        " 1.1643828125, 2.017234375, 0, -1.081390625,",
        " 0, 0, 0, 1",
        ");",
        "void main(void) {",
        " gl_FragColor = vec4( texture2D(YTexture, vTextureCoord).x, texture2D(UTexture, vTextureCoord).x, texture2D(VTexture, vTextureCoord).x, 1) * YUV2RGB;",
        "}"
    ].join("\n");

Texture.prototype.bind = function(n, program, name) {
    var gl = this.gl;
    gl.activeTexture([gl.TEXTURE0, gl.TEXTURE1, gl.TEXTURE2][n]);
    gl.bindTexture(gl.TEXTURE_2D, this.texture);
    gl.uniform1i(gl.getUniformLocation(program, name), n);
}

Texture.prototype.fill = function(width, height, data) {
    var gl = this.gl;
    gl.bindTexture(gl.TEXTURE_2D, this.texture);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.LUMINANCE, width, height, 0, gl.LUMINANCE, gl.UNSIGNED_BYTE, data);
}

function Texture(gl) {
    this.gl = gl;
    this.texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, this.texture);

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);

    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
}

InitDrawer = function(canvas){
    Module.onRuntimeInitialized = function () {
        if (Module.ccall('init_decoder', null, [null], [null]) < 1){
            return -1;
        }
    };

    if (canvas == null){
        return -1;
    }
    yuvCanvas = canvas;

    gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
    if (gl == null){
        return -1;
    } 

    var program = gl.createProgram();

    var vertexShader = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertexShader, vertexShaderSource);
    gl.compileShader(vertexShader);

    var fragmentShader = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragmentShader, fragmentShaderSource);
    gl.compileShader(fragmentShader);

    gl.attachShader(program, vertexShader);
    gl.attachShader(program, fragmentShader);
    gl.linkProgram(program);
    gl.useProgram(program);
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
        return -1;
    }

    var vertexPositionAttribute = gl.getAttribLocation(program, "aVertexPosition");
    gl.enableVertexAttribArray(vertexPositionAttribute);
    var textureCoordAttribute = gl.getAttribLocation(program, "aTextureCoord");
    gl.enableVertexAttribArray(textureCoordAttribute);

    var verticesBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, verticesBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([1.0, 1.0, 0.0, -1.0, 1.0, 0.0, 1.0, -1.0, 0.0, -1.0, -1.0, 0.0]), gl.STATIC_DRAW);
    gl.vertexAttribPointer(vertexPositionAttribute, 3, gl.FLOAT, false, 0, 0);
    var texCoordBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, texCoordBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([1.0, 1.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0]), gl.STATIC_DRAW);
    gl.vertexAttribPointer(textureCoordAttribute, 2, gl.FLOAT, false, 0, 0);

    gl.y = new Texture(gl);
    gl.u = new Texture(gl);
    gl.v = new Texture(gl);
    gl.y.bind(0, program, "YTexture");
    gl.u.bind(1, program, "UTexture");
    gl.v.bind(2, program, "VTexture");

    return 1;
}

var count = 0;
DrawFrame = function(message, callback = null){
    if (typeof message.data == 'blob' || message.data instanceof Blob){
        var reader = new FileReader();
        reader.readAsArrayBuffer(message.data);
        reader.onloadend = function(e){
            let pakBuffer = new Uint8Array(reader.result, 0, reader.result.byteLength),
                pakBufferPtr = Module._malloc(pakBuffer.length);

            Module.HEAP8.set(pakBuffer, pakBufferPtr);
            let imgPtr = Module.ccall("decode_one_frame", null, [typeof pakBufferPtr, typeof pakBuffer.length], [pakBufferPtr, pakBuffer.length]),
                imgWidth = Module.HEAPU32[imgPtr / 4],
                imgHeight = Module.HEAPU32[imgPtr / 4 + 1],
                imgBufferPtr = Module.HEAPU32[imgPtr / 4 + 2];

            imgBuffer = Module.HEAPU8.subarray(imgBufferPtr, imgBufferPtr + imgWidth * imgHeight * 3 / 2);

            if (imgWidth * imgHeight > 0 ){
                gl.viewport(0, 0, yuvCanvas.width, yuvCanvas.height);
                gl.clearColor(0.0, 0.0, 0.0, 0.0);
                gl.clear(gl.COLOR_BUFFER_BIT);

                gl.y.fill(imgWidth, imgHeight, imgBuffer.subarray(0, imgWidth*imgHeight));
                gl.u.fill(imgWidth >> 1, imgHeight >> 1, imgBuffer.subarray(imgWidth*imgHeight, imgWidth*imgHeight*5/4));
                gl.v.fill(imgWidth >> 1, imgHeight >> 1, imgBuffer.subarray(imgWidth*imgHeight*5/4, imgBuffer.length));

                gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
            }

            Module._free(pakBufferPtr);
            Module._free(imgBufferPtr);

            if (callback != null){
                callback();
            }
        }
    }
}

ShutdownDrawer = function(){
    Module.ccall('shutdown_decocer', null, null, null);
}