/**
 * 本示例按照雷神的源码修改一点内容并增加一些备注
 *
 * 本程序使用OpenGL播放YUV视频像素数据。本程序支持YUV420P的
 * 像素数据作为输入，经过转换后输出到屏幕上。其中用到了多种
 * 技术，例如Texture，Shader等，是一个相对比较复杂的例子。
 * 适合有一定OpenGL基础的初学者学习。
 *
 * 函数调用步骤如下：
 *
 * [初始化]
 * glutInit()：初始化glut库。
 * glutInitDisplayMode()：设置显示模式。
 * glutCreateWindow()：创建一个窗口。
 * glewInit()：初始化glew库。
 * glutDisplayFunc()：设置绘图函数（重绘的时候调用）。
 * glutTimerFunc()：设置定时器。
 * InitShaders()：设置Shader。包含了一系列函数，暂不列出。
 * glutMainLoop()：进入消息循环。
 *
 * [循环渲染数据]
 * glActiveTexture()：
 * glBindTexture()：
 * glTexImage2D()：
 * glUniform1i()：
 * glDrawArrays():绘制。
 * glutSwapBuffers()：显示。
 */

#include <stdio.h>

#include "glew.h"
#include "glut.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

// 选择一种纹理模式（设置“1”）
// 默认纹理
#define TEXTURE_DEFAULT   1
// 旋转纹理
#define TEXTURE_ROTATE    0
// 显示一半纹理
#define TEXTURE_HALF      0

// YUV视频文件
FILE* video_file = NULL;
// YUV视频的宽高
const int pixel_w = 320, pixel_h = 180;
// 每一帧的字节数：w * h * 3 / 2 = w * h * 1.5
// 原理如下：yuv420p是临近的4个像素（一个2x2正方形）共用一个色差分量u和另一个色差分量v，亮度分量y则是每个像素各用各自的。
//           算下来，一组2x2的像素，需要4个亮度y，1个色差u，1个色差v，共6个字节，平均每像素1.5字节。
unsigned char buf[pixel_w * pixel_h * 3 / 2];
// plane[3]分别表示YUV三个缓存区
// 如果图像的宽高分别是w和h，yuv420p的数据排列如下：
// 1，y在前面，共wh字节。
// 2，接着u，共wh / 4字节，宽高都是y的一半。
// 3，接着v，与u一样。
// 4，如果是视频，则一帧帧按顺序排，第一帧的yuv，第二帧的yuv，...，最后一帧的yuv，每一帧都是wh * 3 / 2字节。
// 格式非常规整，容易记。
unsigned char* plane[3];


#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4


char* textFileRead(char* filename)
{
    char* s = (char*)malloc(8000);
    memset(s, 0, 8000);
    FILE* infile = fopen(filename, "rb");
    int len = fread(s, 1, 8000, infile);
    fclose(infile);
    s[len] = 0;
    return s;
}


GLuint p;
GLuint id_y, id_u, id_v; // 纹理id
GLuint textureUniformY, textureUniformU, textureUniformV;
/// <summary>
/// 初始化 Shader
/// </summary>
void InitShaders()
{
    GLint vertCompiled, fragCompiled, linked;

    /*-----------------------------（1）  创建一个Shader对象-----------------------------------------*/
    //（1）创建Shader对象（着色器）
    GLint v, f;
    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);
    //（2）读取文件内容,GLSL的语法
    const char* vs, * fs;
    vs = textFileRead("Shader.vsh");    //Vertex Shader（顶点着色器）
    fs = textFileRead("Shader.fsh");    //Fragment Shader（片元着色器）
    //（3）给Shader实例指定源码。
    glShaderSource(v, 1, &vs, NULL);
    glShaderSource(f, 1, &fs, NULL);
    //（4）编译Shader对象
    glCompileShader(v);
    glCompileShader(f);
    //（5）从Shader对象返回一个参数，用来检测着色器编译是否成功。
    glGetShaderiv(v, GL_COMPILE_STATUS, &vertCompiled);    
    glGetShaderiv(f, GL_COMPILE_STATUS, &fragCompiled);

    /*-------------------------------（2）  创建一个Program对象--------------------------------------*/
    //（1）创建program对象（类似于C链接器）
    p = glCreateProgram();
    //（2）绑定shader到program对象中
    glAttachShader(p, v);
    glAttachShader(p, f);
    //（3）将顶点属性（位置、纹理、颜色、法线）索引与着色器中的变量名进行绑定，
    //     如此一来就不需要在着色器中定义该变量（会提示重定义），直接使用即可
    glBindAttribLocation(p, ATTRIB_VERTEX, "vertexIn");
    glBindAttribLocation(p, ATTRIB_TEXTURE, "textureIn");   //vertexIn与textureIn是Shader.vsh文件中定义的变量名
    //（3）连接program对象
    glLinkProgram(p);
    //（4）从program对象返回一个参数的值，用来检测是否连接成功。【linked=1：OK】
    glGetProgramiv(p, GL_LINK_STATUS, &linked);
    //（5）使用porgram对象
    glUseProgram(p);
    // 在程序对象成功链接之前，分配给统一变量的实际位置是不知道的。
    //（6）发生链接后，命令glGetUniformLocation可用于获取统一变量的位置
    textureUniformY = glGetUniformLocation(p, "tex_y");
    textureUniformU = glGetUniformLocation(p, "tex_u");
    textureUniformV = glGetUniformLocation(p, "tex_v"); //tex_y、tex_u和tex_v是Shader.fsh文件中定义的变量名

    /*---------------------------------（3）  初始化Texture-----------------------------------------*/
    //（1）定义顶点数组
#if TEXTURE_ROTATE  //旋转纹理
    static const GLfloat vertexVertices[] = {
        -1.0f, -0.5f,
         0.5f, -1.0f,
        -0.5f,  1.0f,
         1.0f,  0.5f,
    };
#else   //默认
    static const GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f,  1.0f,
        1.0f,  1.0f,
    };
#endif

#if TEXTURE_HALF    //显示一半纹理
    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        0.5f,  1.0f,
        0.0f,  0.0f,
        0.5f,  0.0f,
    };
#else   //默认
    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };
#endif
    //（2）设置通用顶点属性数组
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
    //（3）启用属性数组
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
    glEnableVertexAttribArray(ATTRIB_TEXTURE);
    //（4）初始化纹理【1:用来生成纹理的数量,&texture_y:存储纹理索引的数组】
    glGenTextures(1, &id_y);
    //（5）绑定纹理，才能对该纹理进行操作
    glBindTexture(GL_TEXTURE_2D, id_y);
    //（6）纹理过滤函数：确定如何把纹理象素映射成像素
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);       //【GL_TEXTURE_MAG_FILTER: 放大过滤，GL_LINEAR: 线性过滤, 使用距离当前渲染像素中心最近的4个纹素加权平均值.】
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);       //【GL_TEXTURE_MIN_FILTER: 缩小过滤】
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    //【GL_TEXTURE_WRAP_S: S方向上的贴图模式，GL_CLAMP_TO_EDGE：边界处采用纹理边缘自己的的颜色，和边框无关】
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    //【GL_TEXTURE_WRAP_T: T方向上的贴图模式】

    glGenTextures(1, &id_u);
    glBindTexture(GL_TEXTURE_2D, id_u);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &id_v);
    glBindTexture(GL_TEXTURE_2D, id_v);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void display(void) {
    /// <summary>   
    /// 读取 YUV视频 格式文件，一次一帧,一帧wh*3/2个字节
    /// 就是把video_file里面的值读到buf里面，每次读1个字节，一共读(pixel_w * pixel_h * 3 / 2)次，
    /// 或者把video_file里面的值都读完，不到(pixel_w * pixel_h * 3 / 2)次也会停止。
    /// </summary>
    /// <param name="buf">（1）buffer：是一个指针，对fread来说，它是读入数据的存放地址。对fwrite来说，是要输出数据的地址。</param>
    /// <param name="1">（2）size：要读写的字节数；</param>
    /// <param name="pixel_w * pixel_h * 3 / 2">（3）count : 要进行读写多少个size字节的数据项；</param>
    /// <param name="video_file">4）fp : 文件型指针</param>
    if (fread(buf, 1, pixel_w * pixel_h * 3 / 2, video_file) != pixel_w * pixel_h * 3 / 2) {
        /// <summary>
        /// 移动文件内部指针到开头第一个位置
        /// </summary>
        /// <param name="video_file">stream为文件指针</param>
        /// <param name="0">offset为偏移量 ，正数表示正向偏移，负数表示负向偏移</param>
        /// <param name="SEEK_SET">
        /// whence设定从文件的哪里开始偏移,可能取值为：SEEK_CUR、 SEEK_END 或 SEEK_SET
        /// 　SEEK_SET： 文件开头
        ///   SEEK_CUR： 当前位置
        ///   SEEK_END： 文件结尾
        /// </param>
        fseek(video_file, 0, SEEK_SET);
        fread(buf, 1, pixel_w * pixel_h * 3 / 2, video_file);
    }
    //绘制背景颜色【绿色】
    glClearColor(0.0, 255, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    //Y
    glActiveTexture(GL_TEXTURE0);                           //（1）使用glActiveTexture()选择可以由纹理函数进行修改的当前纹理单位
    glBindTexture(GL_TEXTURE_2D, id_y);                     //（2）接着使用glBindTexture()告诉OpenGL下面对纹理的任何操作都是针对它所绑定的纹理对象的
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pixel_w, pixel_h,
        0, GL_RED, GL_UNSIGNED_BYTE, plane[0]);             //（3）使用glTexImage2D()根据指定的参数，生成一个2D纹理（Texture）
    glUniform1i(textureUniformY, 0);                        //（4）使用glUniform()为当前程序对象指定Uniform变量的值
    //U
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, id_u);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pixel_w / 2, 
        pixel_h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, plane[1]);
    glUniform1i(textureUniformU, 1);
    //V
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, id_v);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pixel_w / 2, 
        pixel_h / 2, 0, GL_RED, GL_UNSIGNED_BYTE, plane[2]);
    glUniform1i(textureUniformV, 2);

    //（5）提供绘制功能。当采用顶点数组方式绘制图形时，使用该函数。该函数根据顶点数组中的坐标数据和指定的模式，进行绘制。
    //【arg1：绘制方式，arg2：从数组缓存中的哪一位开始绘制，一般为0，arg3：数组中顶点的数量】
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //采用双缓冲技术绘制
    glutSwapBuffers();
}

void timeFunc(int value) {
    display();
    // 定时器：40ms
    glutTimerFunc(40, timeFunc, 0);
}


int main(int argc, char* argv[])
{
    //打开 YUV420P 格式的文件
    if ((video_file = fopen("test_yuv420p_320x180.yuv", "rb")) == NULL) {
        printf("cannot open this file\n");
        return -1;
    }

    //YUV Data【有点没想明白，这plane的赋值】
    plane[0] = buf;                             // Y
    plane[1] = plane[0] + pixel_w * pixel_h;    // U
    plane[2] = plane[1] + pixel_w * pixel_h / 4;// V

    
    glutInit(&argc, argv);                          // 初始化glut库
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);   // 设置初始显示模式：GLUT_DOUBLE(双缓冲)、GLUT_RGBA    
    
    glutInitWindowPosition(100, 100);               // 设置窗口的位置。可以指定x，y坐标    
    glutInitWindowSize(500, 500);                   // 设置窗口的大小。可以设置窗口的宽，高。    
    glutCreateWindow("Simplest Video Play OpenGL"); // 创建一个窗口。可以指定窗口的标题。
    
    glewInit();         //此函数初始化后，我们就可以在之后的代码里面方便地使用相关的gl函数。

    glutDisplayFunc(&display);      // 设置绘图函数
    glutTimerFunc(40, timeFunc, 0); // 设置定时器,millis：定时的时间，单位是毫秒。1秒=1000毫秒。(*func)(int value)：用于指定定时器调用的函数。value：给回调函数传参。比较高端，没有接触过。

    InitShaders();      // 初始化Shader
    glutMainLoop();     // 进入GLUT事件处理循环，让所有的与“事件”有关的函数调用无限循环

    return 0;
}






