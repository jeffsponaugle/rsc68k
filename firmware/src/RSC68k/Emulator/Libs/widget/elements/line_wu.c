#include <math.h>
#include "Startup/app.h"
#include "Libs/widget/elements/elements.h"
#include "Libs/window/window.h"

#define RETURN_IF(var)  if (var) return

#define RED_MASK (0xF800)
#define GREEN_MASK (0x07E0)
#define BLUE_MASK (0x001F)

#define	INTENSITY_LEVELS 32

#define INTENSITY_BITS (3)
#define NUM_LEVELS (1<<INTENSITY_BITS)

void LineInit(void)
{
}

// Draw vector line, 16 bit depth
// mode:  Translucent Anti-Aliased
// static void _VectorDraw_16_TV_AA( INT32 x1, INT32 y1, INT32 x2, INT32 y2, UINT32 pen, INT32 intensity)

void LineDrawAA(SWindow *psWindow,
			    SLine *psLine)
{
	INT32 x1, y1, x2, y2, vv_dx, vv_dy, vv_xinc;
	UINT32 vv_DstPitch;
	UINT32 vv_DstPitch2;
	UINT16 *vv_pDst = NULL;
	UINT16 vv_colorMap[INTENSITY_LEVELS];
	UINT32 u32Loop;
	UINT32 intensity;
	UINT16 vv_usColor;
	UINT32 vv_pr, vv_pg, vv_pb, vv_r, vv_g, vv_b;
	UINT16 vv_srcPix;
	UINT16 vv_intensShift, vv_errAdj, vv_errAcc;
	UINT16 vv_errAccTemp, vv_weight, vv_weightCompMask;
	INT32 vv_iTmp;
	UINT32 u32Index = 0;

	vv_DstPitch = psWindow->psWindowImage->u32Pitch;
	intensity = psLine->u32Intensity;

	x1 = psLine->s32X0;
	y1 = psLine->s32Y0;
	x2 = psLine->s32X1;
	y2 = psLine->s32Y1;

	// Now create a color map

	u32Index = 0;
	for (u32Loop = 0; u32Loop < (1 << 8); u32Loop += 8)
	{
		UINT8 u8Red, u8Green, u8Blue;

		u8Red = ((psLine->u16DrawColor >> 11) * u32Loop) >> 8;
		GCASSERT(u8Red <= 0x1f);
		u8Green = (((psLine->u16DrawColor >> 5) & 0x3f) * u32Loop) >> 8;
		GCASSERT(u8Green <= 0x3f);
		u8Blue = ((psLine->u16DrawColor & 0x1f) * u32Loop) >> 8;
		GCASSERT(u8Blue <= 0x1f);

		vv_colorMap[u32Index++] = (u8Red << 11) | (u8Green << 5) | u8Blue;
	}

    /* Make sure the line runs top to bottom */
    if (y1 > y2) 
	{
        INT32 tmp;
        tmp = y1; y1 = y2; y2 = tmp;
        tmp = x1; x1 = x2; x2 = tmp;
    }

    vv_dx = x2 - x1;
    if (vv_dx >= 0) 
	{
      vv_xinc = 1;
    } 
	else 
	{
      vv_xinc = -1;
      vv_dx = -vv_dx; /* make vv_dx positive */
    }

    vv_dy = y2 - y1;

    // Special-case horizontal, vertical, and diagonal lines, which
    // require no weighting because they go right through the center of
    // every pixel 

    if (vv_dy == 0) 
	{
      // Horizontal line 
      // setup pixel ptrs
      if (vv_xinc < 0)
      {
          // left to right only, please.

	//_RasterPixelPtr(vm.backBuffer,x2,y1);
	vv_pDst = psWindow->psWindowImage->pu16ImageData + x2 + (y1 * vv_DstPitch);


    } 
    else 
    {
    
	// vv_pDst = _RasterPixelPtr(vm.backBuffer,x1,y1);
	vv_pDst = psWindow->psWindowImage->pu16ImageData + x1 + (y1 * vv_DstPitch);

	}


    // XXX - I don't think this can actually ever trigger...
    if (intensity >= INTENSITY_LEVELS) 
        intensity = INTENSITY_LEVELS - 1;

    vv_usColor = vv_colorMap[intensity];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);
    
		while (vv_dx-- != 0) {
		vv_pDst += 1;



    // XXX - I don't think this can actually ever trigger...
    if (intensity >= INTENSITY_LEVELS) 
        intensity  = INTENSITY_LEVELS - 1;

    vv_usColor = vv_colorMap[intensity];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);
      }
      return;
    }
    

    if (vv_dx == 0) {
      /* Vertical line */
   
	vv_pDst = psWindow->psWindowImage->pu16ImageData + x1 + (y1 * vv_DstPitch);

    // XXX - I don't think this can actually ever trigger...
    if (intensity >= INTENSITY_LEVELS) 
        intensity  = INTENSITY_LEVELS;

    vv_usColor = vv_colorMap[intensity];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);
    
		do {
		vv_pDst += vv_DstPitch;

    // XXX - I don't think this can actually ever trigger...
    if (intensity >= INTENSITY_LEVELS) 
        intensity  = INTENSITY_LEVELS;

    vv_usColor = vv_colorMap[intensity];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);
    

        } while (--vv_dy != 0);
        return;
    }
    

    // probably only a marginal gain for special-casing diagonal lines... 
    if (vv_dx == vv_dy) {
      /* Diagonal line */
    
	vv_pDst = psWindow->psWindowImage->pu16ImageData + x1 + (y1 * vv_DstPitch);

	vv_DstPitch2 = vv_DstPitch+ vv_xinc;

    // XXX - I don't think this can actually ever trigger...
    if (intensity >= INTENSITY_LEVELS) 
        intensity  = INTENSITY_LEVELS - 1;

    vv_usColor = vv_colorMap[intensity];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);
    
do {
		vv_pDst += vv_DstPitch2;

    // XXX - I don't think this can actually ever trigger...
    if (intensity >= INTENSITY_LEVELS) 
        intensity = INTENSITY_LEVELS - 1;

    vv_usColor = vv_colorMap[intensity];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);
    
		x1 += vv_xinc; y1 += 1;

      } while (--vv_dy != 0);
      return;
    }
    

    /* Line is not horizontal, diagonal, or vertical */
    vv_errAcc = 0;  /* initialize the line error accumulator to 0 */
    

    /* # of bits by which to shift errAcc to get intensity level */
    vv_intensShift = 16 - INTENSITY_BITS;
    /* Mask used to flip all bits in an intensity weighting, producing the
      result (1 - intensity weighting) */
    vv_weightCompMask = NUM_LEVELS - 1;
        
	vv_pDst = psWindow->psWindowImage->pu16ImageData + x1 + (y1 * vv_DstPitch);

	vv_DstPitch2 = vv_DstPitch+ vv_xinc;


    /* Draw the initial pixel, which is always exactly intersected by
      the line and so needs no weighting */

    // XXX - I don't think this can actually ever trigger...
    if (intensity >= INTENSITY_LEVELS) 
        intensity = INTENSITY_LEVELS - 1;

    vv_usColor = vv_colorMap[intensity];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);

    /* Is this an X-major or Y-major line? */
    if (vv_dy > vv_dx) {
      /* Y-major line; calculate 16-bit fixed-point fractional part of a
         pixel that X advances each time Y advances 1 pixel, truncating the
         result so that we won't overrun the endpoint along the X axis */
      vv_errAdj = (UINT16) (((unsigned long) vv_dx << 16) / (unsigned long) vv_dy);
      /* Draw all pixels other than the first and last */

      while (--vv_dy) {
         vv_errAccTemp = vv_errAcc;   /* remember currrent accumulated error */
         vv_errAcc += vv_errAdj;      /* calculate error for next pixel */
         if (vv_errAcc <= vv_errAccTemp) {
            /* The error accumulator turned over, so advance the X coord */
    
		vv_pDst += vv_DstPitch2;


            x1 += vv_xinc;
         }
         else
         {
             // y advances every time
    
		vv_pDst += vv_DstPitch;

}
y1++;

         /* The INTENSITY_BITS most significant bits of vv_errAcc give us the
            intensity weighting for this pixel, and the complement of the
            weighting for the paired pixel */
         vv_weight = vv_errAcc >> vv_intensShift;
    
	vv_iTmp = intensity - vv_weight;
	if (vv_iTmp > 0)
{
    // XXX - I don't think this can actually ever trigger...
    if (vv_iTmp >= INTENSITY_LEVELS) 
        vv_iTmp  = INTENSITY_LEVELS;

    vv_usColor = vv_colorMap[vv_iTmp];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);
    }
	vv_iTmp = intensity - (vv_weight ^ vv_weightCompMask);
	if (vv_iTmp > 0)
{
    // XXX - I don't think this can actually ever trigger...
    if (vv_iTmp >= INTENSITY_LEVELS) 
        vv_iTmp  = INTENSITY_LEVELS;

    vv_usColor = vv_colorMap[vv_iTmp];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst+vv_xinc);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst+vv_xinc) = (UINT16) (vv_r | vv_g | vv_b);
    }


      }
      /* Draw the final pixel, which is always exactly intersected by the line
         and so needs no weighting */

	  // XXX - I don't think this can actually ever trigger...
    if (intensity >= INTENSITY_LEVELS) 
        intensity = INTENSITY_LEVELS - 1;

    vv_usColor = vv_colorMap[intensity];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);
      return;
    }
    

    /* It's an X-major line; calculate 16-bit fixed-point fractional part of a
      pixel that Y advances each time X advances 1 pixel, truncating the
      result to avoid overrunning the endpoint along the X axis */
    vv_errAdj = (UINT16) (((unsigned long) vv_dy << 16) / (unsigned long) vv_dx);
    /* Draw all pixels other than the first and last */
    while (--vv_dx) {
      vv_errAccTemp = vv_errAcc;   /* remember currrent accumulated error */
      vv_errAcc += vv_errAdj;      /* calculate error for next pixel */
      if (vv_errAcc <= vv_errAccTemp) {
         /* The error accumulator turned over, so advance the Y coord */
    
		vv_pDst += vv_DstPitch2;


         y1++;
      } else {
         /* X-major, so always advance X */
    
		vv_pDst += vv_xinc;

	}
	x1 += vv_xinc;

      /* The INTENSITY_BITS most significant bits of vv_errAcc give us the
         intensity weighting for this pixel, and the complement of the
         weighting for the paired pixel */
      vv_weight = vv_errAcc >> vv_intensShift;
    
	vv_iTmp = intensity - vv_weight;
	if (vv_iTmp > 0)
{
    // XXX - I don't think this can actually ever trigger...
    if (vv_iTmp >= INTENSITY_LEVELS) 
        vv_iTmp  = INTENSITY_LEVELS;

    vv_usColor = vv_colorMap[vv_iTmp];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);
    }
	vv_iTmp = intensity - (vv_weight ^ vv_weightCompMask);
	if (vv_iTmp > 0)
{
    // XXX - I don't think this can actually ever trigger...
    if (vv_iTmp >= INTENSITY_LEVELS) 
        vv_iTmp  = INTENSITY_LEVELS;

    vv_usColor = vv_colorMap[vv_iTmp];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst+vv_DstPitch);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst+vv_DstPitch) = (UINT16) (vv_r | vv_g | vv_b);
    }


    }
    /* Draw the final pixel, which is always exactly intersected by the line
      and so needs no weighting */
    // XXX - I don't think this can actually ever trigger...
    if (intensity >= INTENSITY_LEVELS) 
        intensity = INTENSITY_LEVELS - 1;

    vv_usColor = vv_colorMap[intensity];
    vv_pr = vv_usColor & RED_MASK;
    vv_pg = vv_usColor & GREEN_MASK;
    vv_pb = vv_usColor & BLUE_MASK;

    // get src pixel
    vv_srcPix = *(vv_pDst);
    // add dst color to it, clipping.
    vv_r = (vv_srcPix & RED_MASK) + vv_pr;
    if (vv_r > RED_MASK) vv_r = RED_MASK;
    vv_g = (vv_srcPix & GREEN_MASK) + vv_pg;
    if (vv_g > GREEN_MASK) vv_g = GREEN_MASK;
    vv_b = (vv_srcPix & BLUE_MASK) + vv_pb;
    if (vv_b > BLUE_MASK) vv_b = BLUE_MASK;
    // store pixel.
    *(vv_pDst) = (UINT16) (vv_r | vv_g | vv_b);
}
