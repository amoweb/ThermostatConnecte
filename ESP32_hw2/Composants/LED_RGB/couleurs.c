#include <math.h>
#include "couleurs.h"
#include "esp_log.h"

/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
// Define corrections gamma
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */
#define CORREC_GAMMA_R            (2.8)
#define CORREC_GAMMA_G            (2.8)
#define CORREC_GAMMA_B            (3.5)

/*==========================================================*/
/* conversion HSV en RGB */ /*  */
/*==========================================================*/
void hsv_en_rgb(hsv_t *hsv, rgb_t *rgb) {
	double r = 0, g = 0, b = 0;
	double h = hsv->h, s = hsv->s /100.0, v = hsv->v /100.0;

	if (s == 0) {
		r = v;
		g = v;
		b = v;
	}
	else {
		int i;
		double f, p, q, t;

		if (h == 360)
			h = 0;
		else
			h = h / 60;

		i = (int)trunc(h);
		f = h - i;

		p = v * (1.0 - s);
		q = v * (1.0 - (s * f));
		t = v * (1.0 - (s * (1.0 - f)));

		switch (i) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;
		case 1:
			r = q;
			g = v;
			b = p;
			break;
		case 2:
			r = p;
			g = v;
			b = t;
			break;
		case 3:
			r = p;
			g = q;
			b = v;
			break;
		case 4:
			r = t;
			g = p;
			b = v;
			break;
		default:
			r = v;
			g = p;
			b = q;
			break;
		}
// 	ESP_LOGW("hsv_en_rgb","i [%i], f[%0.3f], p[%0.1f], q[%0.1f], t[%0.1f], r[%0.3f], g[%0.3f], b[%0.3f]", i, f, p, q, t, r, g, b);

	}

	rgb->r = r * 255;
  rgb->g = g * 255;
  rgb->b = b * 255;

	return;
}

/*==========================================================*/
/* conversion RGB en HSV */ /*  */
/*==========================================================*/
hsv_t rgb2hsv(rgb_t rgb)
{
	hsv_t         out;
	double      min, max, delta;

	min = rgb.r < rgb.g ? rgb.r : rgb.g;
	min = min  < rgb.b ? min  : rgb.b;

	max = rgb.r > rgb.g ? rgb.r : rgb.g;
	max = max  > rgb.b ? max  : rgb.b;

	out.v = max;                                // v
	delta = max - min;
	if (delta < 0.00001)
	{
		out.s = 0;
		out.h = 0; 																// undefined, maybe nan?
		return out;
	}
	if( max > 0.0 ) { // NOTE: if Max is == 0, this divide would cause a crash
		out.s = (delta / max);                  	// s
	} else {
			// if max is 0, then r = g = b = 0              
			// s = 0, h is undefined
		out.s = 0.0;
		out.h = 0;                            		// its now undefined
		return out;
	}
	if( rgb.r >= max )                        	// > is bogus, just keeps compilor happy
		out.h = ( rgb.g - rgb.b ) / delta;        // between yellow & magenta
	else
	if( rgb.g >= max )
		out.h = 2.0 + ( rgb.b - rgb.r ) / delta;  // between cyan & yellow
	else
		out.h = 4.0 + ( rgb.r - rgb.g ) / delta;  // between magenta & cyan

	out.h *= 60.0;                              // degrees

	if( out.h < 0.0 )
		out.h += 360.0;

	return out;
	}

/*==========================================================*/
/* correction gamma rgb en fonction de la résolution du timer PWM */ /* 
	- param rgb : valeurs RGB 8 bits
	- param résolution : nb de bits du PWM
	retourne rgb corrigé et affecté de la résolution. 
 */
/*==========================================================*/
void correction_gamma_RGB(rgb_t *rgb, int resolution)
{
	int coeff  = 1 << resolution;
	double r = (rgb->r * coeff) / 255;
	double g = (rgb->g * coeff) / 255;
	double b = (rgb->b * coeff) / 255;
	rgb->r = ((coeff-1) * pow(r / coeff, CORREC_GAMMA_R));
	rgb->g = ((coeff-1) * pow(g / coeff, CORREC_GAMMA_G));
	rgb->b = ((coeff-1) * pow(b / coeff, CORREC_GAMMA_B));
// 	ESP_LOGI("correction_gamma_RGB", "rgb[%0.1f, %0.1f, %0.1f] rgb_corr[%i, %i, %i]", r, g, b, rgb->r, rgb->g, rgb->b);
	return ;
}

/*==========================================================*/
/* conversion HSV en RGB corrigé avec résolution du PWM */ /*  */
/*==========================================================*/
void hsv_en_rgb_corrige(hsv_t *hsv, int resolution, rgb_t *rgb) {
	hsv_en_rgb(hsv, rgb);
	correction_gamma_RGB(rgb, resolution);
	return;
}

