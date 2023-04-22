#ifndef _IMAGEELEMENT_H_
#define _IMAGEELEMENT_H_

typedef struct SImageElement
{
	SImageGroup *psGroup;		// Image group
} SImageElement;

extern void ImageElementInit(void);
#endif	// #ifndef _IMAGEELEMENT_H_