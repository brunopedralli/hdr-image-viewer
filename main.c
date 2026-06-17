#include <stdio.h>
#include <stdlib.h>
#include <string.h> // Para usar strings
#include <math.h>
#include <float.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_write.h>

// Um pixel RGB (24 bits)
typedef struct
{
    unsigned char r, g, b;
} RGBPixel;

// Um pixel no formato RGBE (unsigned char)
typedef struct
{
    unsigned char r, g, b, e;
} RGBEPixel;

// Um pixel no formato RGBf (float)
typedef struct
{
    float r, g, b;
} RGBFPixel;

typedef struct
{
    int width, height;
    RGBPixel *pixels;
} ImgRGB;

// Uma imagem RGBF
typedef struct
{
    int width, height;
    RGBFPixel *pixels;
} ImgRGBF;

typedef void (*FuncPixel)(RGBFPixel *p, float param);

// Protótipos
void process(ImgRGBF *in, ImgRGB *out, float stop, float gamma, int tonemapAlgo);
void carregaHeader(FILE *fp, ImgRGBF *img);
void carregaImagem(FILE *fp, ImgRGBF *img);

void aplicaExposicao(RGBFPixel *p, float stop);
void aplicaGama(RGBFPixel *p, float gama);

void tonemapReinhard(ImgRGBF *img);
void tonemapACES(ImgRGBF *img);

int main(int argc, char *argv[])
{
    // Inclua e processe os demais argumentos por linha de comando
    if (argc < 5)
    {
        printf("Uso: hdrvis <imagem.hdf> <stop> <gamma> tonemap: 1=Reinhard 2=ACES>\n");
        printf("Exemplo: ./output/hdrvis images/cathedral.hdf 0 2.2 1\n");
        exit(1);
    }

    float stop = atof(argv[2]);
    float gamma = atof(argv[3]);
    int tonemapAlgo = atoi(argv[4]);

    if (gamma <= 0.0f)
    {
        fprintf(stderr, "Erro: gamma deve ser > 0 (recebido: %.3f)\n", gamma);
        exit(1);
    }
    if (tonemapAlgo != 1 && tonemapAlgo != 2)
    {
        fprintf(stderr, "Erro: tonemap deve ser 1 (Reinhard) ou 2 (ACES) (recebido: %d)\n", tonemapAlgo);
        exit(1);
    }

    // Imagens de entrada e saída
    ImgRGBF entrada;
    ImgRGB saida;

    //
    // PASSO 1: Leitura da imagem
    // A leitura do header deve ser feita na função carregaHeader
    FILE *arq = fopen(argv[1], "rb"); // abre em formato binário
    if (!arq)
    {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'\n", argv[1]);
        exit(1);
    }

    carregaHeader(arq, &entrada);
    // Exibe as dimensões na tela, para conferência
    printf("Entrada   : %s %d x %d\n", argv[1], entrada.width, entrada.height);

    // A leitura do restante da imagem deve ser feita na função carregaImagem
    carregaImagem(arq, &entrada);
    fclose(arq);

    // Cria imagem de saída e "zera" ela
    int tam = entrada.width * entrada.height;
    saida.width = entrada.width;
    saida.height = entrada.height;
    saida.pixels = malloc(tam * sizeof(RGBPixel));
    if (!saida.pixels)
    {
        fprintf(stderr, "Erro: falha ao alocar imagem de saida (%d pixels)\n", tam);
        exit(1);
    }
    memset(saida.pixels, 0, tam * sizeof(RGBPixel));

    printf("Stop : %.1f Gamma: %.1f ToneMap: %s\n", stop, gamma, tonemapAlgo == 1 ? "Reinhard" : "ACES");
    printf("Processando...\n");
    // Aplica todo o processamento necessário (exposição, etc) na função process
    // (acrescente mais parâmetros conforme a necessidade)
    clock_t inicio = clock();
    process(&entrada, &saida, stop, gamma, tonemapAlgo);
    clock_t fim = clock();
    printf("Tempo: %.3f s\n", (double)(fim - inicio) / CLOCKS_PER_SEC);

    char *nome = strrchr(argv[1], '/');
    nome = nome ? nome + 1 : argv[1];
    char *ext = strrchr(nome, '.');

    if (ext)
        *ext = '\0';

    char out[1024];
    snprintf(out, sizeof(out), "output/%s_saida.jpg", nome);

    // Grava a imagem de saída como JPEG para conferência (usando a função stbi_write_jpg, com qualidade 90)
    stbi_write_jpg(out, saida.width, saida.height, 3, saida.pixels, 90);
    printf("Saida gravada em %s\n", out);

    free(entrada.pixels);
    free(saida.pixels);
    return 0;
}

// Esta função deverá ser utilizada para apenas ler o conteúdo do header
// e extrair a largura e altura da imagem
void carregaHeader(FILE *fp, ImgRGBF *img)
{
    char magic[4] = {0};
    if (fread(magic, 1, 3, fp) != 3)
    {
        fprintf(stderr, "Erro: arquivo truncado ao ler magic number\n");
        exit(1);
    }

    if (strncmp(magic, "HDF", 3) != 0)
    {
        fprintf(stderr, "Erro: arquivo nao e HDF valido (magic='%s')\n", magic);
        exit(1);
    }

    unsigned int w, h;
    if (fread(&w, sizeof(unsigned int), 1, fp) != 1 ||
        fread(&h, sizeof(unsigned int), 1, fp) != 1)
    {
        fprintf(stderr, "Erro: arquivo truncado ao ler dimensoes\n");
        exit(1);
    }

    img->width = (int)w;
    img->height = (int)h;
    img->pixels = NULL;
}

// Esta função deverá ser utilizada para carregar o restante
// da imagem (após ler o header e extrair a largura e altura corretamente)
// (não esqueça de alocar memória para os bytes no formato RGBE (unsigned char)
// e também para os bytes no formato RGBF (float) )
void carregaImagem(FILE *fp, ImgRGBF *img)
{
    int tam = img->width * img->height;
    img->pixels = malloc(tam * sizeof(RGBFPixel));
    if (!img->pixels)
    {
        fprintf(stderr, "Erro: falha ao alocar imagem de entrada (%d pixels)\n", tam);
        exit(1);
    }

    for (int i = 0; i < tam; i++)
    {
        RGBEPixel rgbe;
        if (fread(&rgbe, sizeof(RGBEPixel), 1, fp) != 1)
        {
            fprintf(stderr, "Erro: arquivo truncado no pixel %d de %d\n", i, tam);
            exit(1);
        }

        // Fator de conversão: f = 2^(e - 128) / 255
        float f = ldexp(1.0f, (int)rgbe.e - 128) / 255.0f;

        img->pixels[i].r = rgbe.r * f;
        img->pixels[i].g = rgbe.g * f;
        img->pixels[i].b = rgbe.b * f;
    }
}
// Aplica fator de exposição: multiplica por 2^stop
void aplicaExposicao(RGBFPixel *p, float stop)
{
    float fator = powf(2.0f, stop);
    p->r *= fator;
    p->g *= fator;
    p->b *= fator;
}
// Aplica correção gama: v^(1/gamma)
void aplicaGama(RGBFPixel *p, float gamma)
{
    float exp = 1.0f / gamma;
    p->r = powf(p->r, exp);
    p->g = powf(p->g, exp);
    p->b = powf(p->b, exp);
}

void tonemapReinhard(ImgRGBF *img)
{
    int tam = img->width * img->height;

    float Lwhite = 0.0f; // encontra a maior luminancia
    for (int i = 0; i < tam; i++)
    {
        RGBFPixel *p = &img->pixels[i];
        float L = 0.2126f * p->r + 0.7152f * p->g + 0.0722f * p->b;
        if (L > Lwhite)
        {
            Lwhite = L;
        }
    }
    float Lw2 = Lwhite * Lwhite;

    for (int i = 0; i < tam; i++)
    {
        RGBFPixel *p = &img->pixels[i];

        float L = 0.2126f * p->r + 0.7152f * p->g + 0.0722f * p->b;
        float Ld = (L * (1.0f + L / Lw2)) / (1.0f + L);
        float scale = Ld / L;

        p->r *= scale;
        p->g *= scale;
        p->b *= scale;

        if (p->r > 1.0f) p->r = 1.0f;
        if (p->r < 0.0f) p->r = 0.0f;
        if (p->g > 1.0f) p->g = 1.0f;
        if (p->g < 0.0f) p->g = 0.0f;
        if (p->b > 1.0f) p->b = 1.0f;
        if (p->b < 0.0f) p->b = 0.0f;
    }
}

void tonemapACES(ImgRGBF *img)
{
    int tam = img->width * img->height;
    for (int i = 0; i < tam; i++)
    {
        RGBFPixel *p = &img->pixels[i];

        // Reduz para 60%
        float r = p->r * 0.6f;
        float g = p->g * 0.6f;
        float b = p->b * 0.6f;

        // Fórmula ACES: (x*(a*x+b))/(x*(c*x+d)+e)
        // a=2.51, b=0.03, c=2.43, d=0.59, e=0.14
        p->r = (r * (2.51f * r + 0.03f)) / (r * (2.43f * r + 0.59f) + 0.14f);
        p->g = (g * (2.51f * g + 0.03f)) / (g * (2.43f * g + 0.59f) + 0.14f);
        p->b = (b * (2.51f * b + 0.03f)) / (b * (2.43f * b + 0.59f) + 0.14f);

        if (p->r > 1.0f) p->r = 1.0f;
        if (p->r < 0.0f) p->r = 0.0f;
        if (p->g > 1.0f) p->g = 1.0f;
        if (p->g < 0.0f) p->g = 0.0f;
        if (p->b > 1.0f) p->b = 1.0f;
        if (p->b < 0.0f) p->b = 0.0f;
    }
}

// Executa todo o pipeline de processamento, grava saída em out->pixels
void process(ImgRGBF *in, ImgRGB *out, float stop, float gamma, int tonemapAlgo)
{
    int tam = in->width * in->height;

    FuncPixel fnExposicao = aplicaExposicao;
    FuncPixel fnGama = aplicaGama;

    for (int i = 0; i < tam; i++)
    {
        fnExposicao(&in->pixels[i], stop);
    }

    void (*fnToneMap)(ImgRGBF *) = (tonemapAlgo == 2) ? tonemapACES : tonemapReinhard;
    fnToneMap(in);

    for (int i = 0; i < tam; i++)
    {
        fnGama(&in->pixels[i], gamma);
    }

    for (int i = 0; i < tam; i++)
    {
        out->pixels[i].r = (unsigned char)(in->pixels[i].r * 255.0f);
        out->pixels[i].g = (unsigned char)(in->pixels[i].g * 255.0f);
        out->pixels[i].b = (unsigned char)(in->pixels[i].b * 255.0f);
    }
}
