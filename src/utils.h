/*
 * $Id: utils.h 3065 2014-07-19 16:19:38Z darren_janeczek $
 */

#ifndef UTILS_H
#define UTILS_H

/* The AdjustValue functions used to be #define'd macros, but these are 
 * evil for several reasons, *especially* when they contain multiple
 * statements, and have if statements in them. The macros did both.
 * See http://www.parashift.com/c++-faq-lite/inline-functions.html#faq-9.5
 * for more information. 
 */
inline void AdjustValueMax(int &v, int val, int max) { v += val; if (v > max) v = max; }
inline void AdjustValueMin(int &v, int val, int min) { v += val; if (v < min) v = min; }
inline void AdjustValue(int &v, int val, int max, int min) { v += val; if (v > max) v = max; if (v < min) v = min; }

inline void AdjustValueMax(short &v, int val, int max) { v += val; if (v > max) v = max; }
inline void AdjustValueMin(short &v, int val, int min) { v += val; if (v < min) v = min; }
inline void AdjustValue(short &v, int val, int max, int min) { v += val; if (v > max) v = max; if (v < min) v = min; }

inline void AdjustValueMax(unsigned short &v, int val, int max) { v += val; if (v > max) v = max; }
inline void AdjustValueMin(unsigned short &v, int val, int min) { v += val; if (v < min) v = min; }
inline void AdjustValue(unsigned short &v, int val, int max, int min) { v += val; if (v > max) v = max; if (v < min) v = min; }

#endif
