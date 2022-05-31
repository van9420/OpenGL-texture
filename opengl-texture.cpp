/**
 * ��ʾ�����������Դ���޸�һ�����ݲ�����һЩ��ע
 *
 * ������ʹ��OpenGL����YUV��Ƶ�������ݡ�������֧��YUV420P��
 * ����������Ϊ���룬����ת�����������Ļ�ϡ������õ��˶���
 * ����������Texture��Shader�ȣ���һ����ԱȽϸ��ӵ����ӡ�
 * �ʺ���һ��OpenGL�����ĳ�ѧ��ѧϰ��
 *
 * �������ò������£�
 *
 * [��ʼ��]
 * glutInit()����ʼ��glut�⡣
 * glutInitDisplayMode()��������ʾģʽ��
 * glutCreateWindow()������һ�����ڡ�
 * glewInit()����ʼ��glew�⡣
 * glutDisplayFunc()�����û�ͼ�������ػ��ʱ����ã���
 * glutTimerFunc()�����ö�ʱ����
 * InitShaders()������Shader��������һϵ�к������ݲ��г���
 * glutMainLoop()��������Ϣѭ����
 *
 * [ѭ����Ⱦ����]
 * glActiveTexture()��
 * glBindTexture()��
 * glTexImage2D()��
 * glUniform1i()��
 * glDrawArrays():���ơ�
 * glutSwapBuffers()����ʾ��
 */

#include <stdio.h>

#include "glew.h"
#include "glut.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

// ѡ��һ������ģʽ�����á�1����
// Ĭ������
#define TEXTURE_DEFAULT   1
// ��ת����
#define TEXTURE_ROTATE    0
// ��ʾһ������
#define TEXTURE_HALF      0

// YUV��Ƶ�ļ�
FILE* video_file = NULL;
// YUV��Ƶ�Ŀ��
const int pixel_w = 320, pixel_h = 180;
// ÿһ֡���ֽ�����w * h * 3 / 2 = w * h * 1.5
// ԭ�����£�yuv420p���ٽ���4�����أ�һ��2x2�����Σ�����һ��ɫ�����u����һ��ɫ�����v�����ȷ���y����ÿ�����ظ��ø��Եġ�
//           ��������һ��2x2�����أ���Ҫ4������y��1��ɫ��u��1��ɫ��v����6���ֽڣ�ƽ��ÿ����1.5�ֽڡ�
unsigned char buf[pixel_w * pixel_h * 3 / 2];
// plane[3]�ֱ��ʾYUV����������
// ���ͼ��Ŀ�߷ֱ���w��h��yuv420p�������������£�
// 1��y��ǰ�棬��wh�ֽڡ�
// 2������u����wh / 4�ֽڣ���߶���y��һ�롣
// 3������v����uһ����
// 4���������Ƶ����һ֡֡��˳���ţ���һ֡��yuv���ڶ�֡��yuv��...�����һ֡��yuv��ÿһ֡����wh * 3 / 2�ֽڡ�
// ��ʽ�ǳ����������׼ǡ�
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
GLuint id_y, id_u, id_v; // ����id
GLuint textureUniformY, textureUniformU, textureUniformV;
/// <summary>
/// ��ʼ�� Shader
/// </summary>
void InitShaders()
{
    GLint vertCompiled, fragCompiled, linked;

    /*-----------------------------��1��  ����һ��Shader����-----------------------------------------*/
    //��1������Shader������ɫ����
    GLint v, f;
    v = glCreateShader(GL_VERTEX_SHADER);
    f = glCreateShader(GL_FRAGMENT_SHADER);
    //��2����ȡ�ļ�����,GLSL���﷨
    const char* vs, * fs;
    vs = textFileRead("Shader.vsh");    //Vertex Shader��������ɫ����
    fs = textFileRead("Shader.fsh");    //Fragment Shader��ƬԪ��ɫ����
    //��3����Shaderʵ��ָ��Դ�롣
    glShaderSource(v, 1, &vs, NULL);
    glShaderSource(f, 1, &fs, NULL);
    //��4������Shader����
    glCompileShader(v);
    glCompileShader(f);
    //��5����Shader���󷵻�һ�����������������ɫ�������Ƿ�ɹ���
    glGetShaderiv(v, GL_COMPILE_STATUS, &vertCompiled);    
    glGetShaderiv(f, GL_COMPILE_STATUS, &fragCompiled);

    /*-------------------------------��2��  ����һ��Program����--------------------------------------*/
    //��1������program����������C��������
    p = glCreateProgram();
    //��2����shader��program������
    glAttachShader(p, v);
    glAttachShader(p, f);
    //��3�����������ԣ�λ�á�������ɫ�����ߣ���������ɫ���еı��������а󶨣�
    //     ���һ���Ͳ���Ҫ����ɫ���ж���ñ���������ʾ�ض��壩��ֱ��ʹ�ü���
    glBindAttribLocation(p, ATTRIB_VERTEX, "vertexIn");
    glBindAttribLocation(p, ATTRIB_TEXTURE, "textureIn");   //vertexIn��textureIn��Shader.vsh�ļ��ж���ı�����
    //��3������program����
    glLinkProgram(p);
    //��4����program���󷵻�һ��������ֵ����������Ƿ����ӳɹ�����linked=1��OK��
    glGetProgramiv(p, GL_LINK_STATUS, &linked);
    //��5��ʹ��porgram����
    glUseProgram(p);
    // �ڳ������ɹ�����֮ǰ�������ͳһ������ʵ��λ���ǲ�֪���ġ�
    //��6���������Ӻ�����glGetUniformLocation�����ڻ�ȡͳһ������λ��
    textureUniformY = glGetUniformLocation(p, "tex_y");
    textureUniformU = glGetUniformLocation(p, "tex_u");
    textureUniformV = glGetUniformLocation(p, "tex_v"); //tex_y��tex_u��tex_v��Shader.fsh�ļ��ж���ı�����

    /*---------------------------------��3��  ��ʼ��Texture-----------------------------------------*/
    //��1�����嶥������
#if TEXTURE_ROTATE  //��ת����
    static const GLfloat vertexVertices[] = {
        -1.0f, -0.5f,
         0.5f, -1.0f,
        -0.5f,  1.0f,
         1.0f,  0.5f,
    };
#else   //Ĭ��
    static const GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f,  1.0f,
        1.0f,  1.0f,
    };
#endif

#if TEXTURE_HALF    //��ʾһ������
    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        0.5f,  1.0f,
        0.0f,  0.0f,
        0.5f,  0.0f,
    };
#else   //Ĭ��
    static const GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };
#endif
    //��2������ͨ�ö�����������
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);
    //��3��������������
    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);
    glEnableVertexAttribArray(ATTRIB_TEXTURE);
    //��4����ʼ������1:�����������������,&texture_y:�洢�������������顿
    glGenTextures(1, &id_y);
    //��5�����������ܶԸ�������в���
    glBindTexture(GL_TEXTURE_2D, id_y);
    //��6��������˺�����ȷ����ΰ���������ӳ�������
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);       //��GL_TEXTURE_MAG_FILTER: �Ŵ���ˣ�GL_LINEAR: ���Թ���, ʹ�þ��뵱ǰ��Ⱦ�������������4�����ؼ�Ȩƽ��ֵ.��
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);       //��GL_TEXTURE_MIN_FILTER: ��С���ˡ�
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    //��GL_TEXTURE_WRAP_S: S�����ϵ���ͼģʽ��GL_CLAMP_TO_EDGE���߽紦���������Ե�Լ��ĵ���ɫ���ͱ߿��޹ء�
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    //��GL_TEXTURE_WRAP_T: T�����ϵ���ͼģʽ��

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
    /// ��ȡ YUV��Ƶ ��ʽ�ļ���һ��һ֡,һ֡wh*3/2���ֽ�
    /// ���ǰ�video_file�����ֵ����buf���棬ÿ�ζ�1���ֽڣ�һ����(pixel_w * pixel_h * 3 / 2)�Σ�
    /// ���߰�video_file�����ֵ�����꣬����(pixel_w * pixel_h * 3 / 2)��Ҳ��ֹͣ��
    /// </summary>
    /// <param name="buf">��1��buffer����һ��ָ�룬��fread��˵�����Ƕ������ݵĴ�ŵ�ַ����fwrite��˵����Ҫ������ݵĵ�ַ��</param>
    /// <param name="1">��2��size��Ҫ��д���ֽ�����</param>
    /// <param name="pixel_w * pixel_h * 3 / 2">��3��count : Ҫ���ж�д���ٸ�size�ֽڵ������</param>
    /// <param name="video_file">4��fp : �ļ���ָ��</param>
    if (fread(buf, 1, pixel_w * pixel_h * 3 / 2, video_file) != pixel_w * pixel_h * 3 / 2) {
        /// <summary>
        /// �ƶ��ļ��ڲ�ָ�뵽��ͷ��һ��λ��
        /// </summary>
        /// <param name="video_file">streamΪ�ļ�ָ��</param>
        /// <param name="0">offsetΪƫ���� ��������ʾ����ƫ�ƣ�������ʾ����ƫ��</param>
        /// <param name="SEEK_SET">
        /// whence�趨���ļ������￪ʼƫ��,����ȡֵΪ��SEEK_CUR�� SEEK_END �� SEEK_SET
        /// ��SEEK_SET�� �ļ���ͷ
        ///   SEEK_CUR�� ��ǰλ��
        ///   SEEK_END�� �ļ���β
        /// </param>
        fseek(video_file, 0, SEEK_SET);
        fread(buf, 1, pixel_w * pixel_h * 3 / 2, video_file);
    }
    //���Ʊ�����ɫ����ɫ��
    glClearColor(0.0, 255, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    //Y
    glActiveTexture(GL_TEXTURE0);                           //��1��ʹ��glActiveTexture()ѡ������������������޸ĵĵ�ǰ����λ
    glBindTexture(GL_TEXTURE_2D, id_y);                     //��2������ʹ��glBindTexture()����OpenGL�����������κβ���������������󶨵���������
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, pixel_w, pixel_h,
        0, GL_RED, GL_UNSIGNED_BYTE, plane[0]);             //��3��ʹ��glTexImage2D()����ָ���Ĳ���������һ��2D����Texture��
    glUniform1i(textureUniformY, 0);                        //��4��ʹ��glUniform()Ϊ��ǰ�������ָ��Uniform������ֵ
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

    //��5���ṩ���ƹ��ܡ������ö������鷽ʽ����ͼ��ʱ��ʹ�øú������ú������ݶ��������е��������ݺ�ָ����ģʽ�����л��ơ�
    //��arg1�����Ʒ�ʽ��arg2�������黺���е���һλ��ʼ���ƣ�һ��Ϊ0��arg3�������ж����������
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    //����˫���弼������
    glutSwapBuffers();
}

void timeFunc(int value) {
    display();
    // ��ʱ����40ms
    glutTimerFunc(40, timeFunc, 0);
}


int main(int argc, char* argv[])
{
    //�� YUV420P ��ʽ���ļ�
    if ((video_file = fopen("test_yuv420p_320x180.yuv", "rb")) == NULL) {
        printf("cannot open this file\n");
        return -1;
    }

    //YUV Data���е�û�����ף���plane�ĸ�ֵ��
    plane[0] = buf;                             // Y
    plane[1] = plane[0] + pixel_w * pixel_h;    // U
    plane[2] = plane[1] + pixel_w * pixel_h / 4;// V

    
    glutInit(&argc, argv);                          // ��ʼ��glut��
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);   // ���ó�ʼ��ʾģʽ��GLUT_DOUBLE(˫����)��GLUT_RGBA    
    
    glutInitWindowPosition(100, 100);               // ���ô��ڵ�λ�á�����ָ��x��y����    
    glutInitWindowSize(500, 500);                   // ���ô��ڵĴ�С���������ô��ڵĿ��ߡ�    
    glutCreateWindow("Simplest Video Play OpenGL"); // ����һ�����ڡ�����ָ�����ڵı��⡣
    
    glewInit();         //�˺�����ʼ�������ǾͿ�����֮��Ĵ������淽���ʹ����ص�gl������

    glutDisplayFunc(&display);      // ���û�ͼ����
    glutTimerFunc(40, timeFunc, 0); // ���ö�ʱ��,millis����ʱ��ʱ�䣬��λ�Ǻ��롣1��=1000���롣(*func)(int value)������ָ����ʱ�����õĺ�����value�����ص��������Ρ��Ƚϸ߶ˣ�û�нӴ�����

    InitShaders();      // ��ʼ��Shader
    glutMainLoop();     // ����GLUT�¼�����ѭ���������е��롰�¼����йصĺ�����������ѭ��

    return 0;
}






