#ifndef COULEUR_H
#define COULEUR_H

typedef struct RGB {
	int r;
	int g;
	int b;
} rgb_t;

typedef struct HSV {
	double h;
	double s;
	double v;
} hsv_t;

void correction_gamma_RGB(rgb_t *rgb, int resolution);
void hsv_en_rgb_corrige(hsv_t *hsv, int resolution, rgb_t *rgb);

#define OFF_RGB         0, 0, 0

// RED       - R: 255, G: 0  , B: 0
//             H: 0  , S: 100, V: 100
#define RED           	255, 0, 0

// YELLOW    - R: 255, G: 255, B: 0
//             H: 60 , S: 100, V: 100
#define YELLOW					255, 255, 0

// GREEN    - R: 0  , G: 255, B: 0
//            H: 120, S: 100, V: 100
#define GREEN						0, 255, 0

// CYAN      - R: 0  , G: 255, B: 255
//             H: 180, S: 100, V: 100
#define CYAN						0, 255, 255

// BLUE     - R: 0  , G: 0   , B: 255
//            H: 240, S: 100, V: 100
#define BLUE						0, 0, 255

// MAGENTA   - R: 255, G: 0  , B: 255
//             H: 300, S: 100, V: 100
#define MAGENTA					255, 0, 255

// ORANGE   - R: 255, G: 128, B: 0
//            H: 30 , S: 100, V: 100
#define ORANGE					255, 128, 0

// ROSE PINK - R: 255, G: 0  , B: 128
//             H: 330, S: 100, V: 100
#define ROSE						255, 0, 128

// BLUEISH PURPLE - R: 178, G: 102, B: 255
//                  H: 270, S: 60, V: 100
#define BLUEISH_PURPLE				178, 102, 255


#endif // COULEUR_H
