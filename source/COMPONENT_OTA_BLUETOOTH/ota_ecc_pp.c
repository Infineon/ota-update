/*
 * Copyright 2023, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 */

/*
 *  Simple pairing algorithms
 */

#include "ota_multprecision.h"
#include "ota_ecc_pp.h"
#include <stdio.h>
#include <string.h>

void ota_ECC_Add(Point *r, Point *p, PointAff *q);
void ota_ECC_Double(Point *q, Point *p);

ota_EC ota_curve = {
    { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x0, 0x0, 0x0, 0x1, 0xFFFFFFFF },
    { 0xFC632551, 0xF3B9CAC2, 0xA7179E84, 0xBCE6FAAD, 0xFFFFFFFF, 0xFFFFFFFF, 0x0, 0xFFFFFFFF },
    {
        { 0xd898c296, 0xf4a13945, 0x2deb33a0, 0x77037d81, 0x63a440f2, 0xf8bce6e5, 0xe12c4247, 0x6b17d1f2},
        { 0x37bf51f5, 0xcbb64068, 0x6b315ece, 0x2bce3357, 0x7c0f9e16, 0x8ee7eb4a, 0xfe1a7f9b, 0x4fe342e2},
        { 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
    }
};

UINT32 ota_nprime[KEY_LENGTH_DWORDS] = { 0xEE00BC4F, 0xCCD1C8AA, 0x7D74D2E4, 0x48C94408, 0xC588C6F6, 0x50FE77EC, 0xA9D6281C, 0x60D06633};

UINT32 ota_rr[KEY_LENGTH_DWORDS] = { 0xBE79EEA2, 0x83244C95, 0x49BD6FA6, 0x4699799C, 0x2B6BEC59, 0x2845B239, 0xF3D95620, 0x66E12D94 };

void ota_InitPoint(Point *q)
{
    memset(q, 0, sizeof(Point));
}

void ota_CopyPoint(Point *q, PointAff *p)
{
    memcpy(q, p, sizeof(PointAff));
    memset(q->z, 0, sizeof(q->z));
    q->z[0] = 0x1;
}
// Computing the NAF of a positive integer
void ota_ECC_NAF(UINT8 *naf, UINT32 *NumNAF, DWORD *k)
{
    UINT32    sign;
    int i=0;
    int j;

    while(ota_MP_MostSignBits(k)>=1)            // k>=1
    {
        if(k[0] & 0x01)     // k is odd
        {
            sign=(k[0]&0x03);           // 1 or 3

            // k = k-naf[i]
            if(sign==1)
                k[0]=k[0]&0xFFFFFFFE;
            else
            {
                k[0]=k[0]+1;
                if(k[0]==0)             //overflow
                {
                    j=1;
                    do
                    {
                        k[j]++;
                    }while(k[j++]==0);          //overflow
                }
            }
        }
        else
            sign=0;

        ota_MP_RShift(k, k);

        naf[i / 4] |= (sign) << ((i % 4) * 2);

        i++;
    }

    *NumNAF=i;
}

void ota_ECC_PRJ_TO_AFF(Point* q)
{
    UINT32 w[KEY_LENGTH_DWORDS];

    ota_MP_InvMod(w, q->z, modp);
    ota_MP_MersennsSquaMod(q->z, w);
    ota_MP_MersennsMultMod(q->x, q->x, q->z);

    ota_MP_MersennsMultMod(q->z, q->z, w);
    ota_MP_MersennsMultMod(q->y, q->y, q->z);

    ota_MP_Init(q->z);
    q->z[0]=1;
}

// Binary NAF for point multiplication
void ota_ECC_PM_B_NAF(Point *q, Point *p, DWORD *n)
{
    int i;
    UINT32 sign;
    UINT8 naf[KEY_LENGTH_BITS / 4 +1];
    UINT32 NumNaf;
    PointAff minus_p;
    Point r;

    ota_InitPoint(&r);

    // initialization
    ota_InitPoint(q);
    // -p
    ota_MP_Copy(minus_p.x, (DWORD*)p->x);
    ota_MP_Sub(minus_p.y, modp, (DWORD*)p->y);

    // NAF
    memset(naf, 0, sizeof(naf));
    ota_ECC_NAF(naf, &NumNaf, n);
    for(i=NumNaf-1; i>=0; i--)
    {
        ota_ECC_Double(q, q);

        sign = (naf[i / 4] >> ((i % 4) * 2)) & 0x03;

        if(sign==1)
        {
            ota_ECC_Add(q, q, (PointAff*)p);
        }
        else if(sign == 3)
        {
            ota_ECC_Add(q, q, &minus_p);
        }

    }

    ota_ECC_PRJ_TO_AFF(q);
}

// q=2q, zq of length KEY_LENGTH_DWORDS+1
void ota_ECC_Double(Point *q, Point *p)
{
    DWORD t1[KEY_LENGTH_DWORDS], t2[KEY_LENGTH_DWORDS], t3[KEY_LENGTH_DWORDS];
    DWORD *x1, *x3, *y1, *y3, *z1, *z3;
//  DWORD t4[KEY_LENGTH_DWORDS];

    if(ota_MP_isZero(p->z))
    {
        ota_MP_Init(q->z);
        return;                     // return infinity
    }
    x1=p->x; y1=p->y; z1=p->z;
    x3=q->x; y3=q->y; z3=q->z;

    ota_MP_MersennsSquaMod(t1, z1);             // t1=z1^2
    ota_MP_SubMod(t2, x1, t1);              // t2=x1-t1
    ota_MP_AddMod(t1, x1, t1);              // t1=x1+t1
    ota_MP_MersennsMultMod(t2, t1, t2);         // t2=t2*t1
    ota_MP_LShiftMod(t3, t2);
    ota_MP_AddMod(t2, t3, t2);              // t2=3t2

    ota_MP_MersennsMultMod(z3, y1, z1);         // z3=y1*z1
    ota_MP_LShiftMod(z3, z3);

    ota_MP_MersennsSquaMod(y3, y1);             // y3=y1^2
    ota_MP_LShiftMod(y3, y3);
    ota_MP_MersennsMultMod(t3, y3, x1);         // t3=y3*x1=x1*y1^2
    ota_MP_LShiftMod(t3, t3);
    ota_MP_MersennsSquaMod(y3, y3);             // y3=y3^2=y1^4
    ota_MP_LShiftMod(y3, y3);

    ota_MP_MersennsSquaMod(x3, t2);             // x3=t2^2
    ota_MP_LShiftMod(t1, t3);           // t1=2t3
    ota_MP_SubMod(x3, x3, t1);              // x3=x3-t1
    ota_MP_SubMod(t1, t3, x3);              // t1=t3-x3
    ota_MP_MersennsMultMod(t1, t1, t2);         // t1=t1*t2
    ota_MP_SubMod(y3, t1, y3);              // y3=t1-y3
}

// q=q+p,     p is affine point
void ota_ECC_Add(Point *r, Point *p, PointAff *q)
{
    DWORD t1[KEY_LENGTH_DWORDS], t2[KEY_LENGTH_DWORDS];
    DWORD k[KEY_LENGTH_DWORDS];
    DWORD s[KEY_LENGTH_DWORDS];
    DWORD *x1, *x2, *x3, *y1, *y2, *y3, *z1, *z3;

    x1=p->x; y1=p->y; z1=p->z;
    x2=q->x; y2=q->y;
    x3=r->x; y3=r->y; z3=r->z;

    // if P=infinity, return q
    if(ota_MP_isZero(z1))
    {
        ota_CopyPoint(r, q);
        return;
    }

    ota_MP_MersennsSquaMod(t1, z1);             // t1=z1^2
    ota_MP_MersennsMultMod(t2, z1, t1);         // t2=t1*z1
    ota_MP_MersennsMultMod(t1, x2, t1);         // t1=t1*x2
    ota_MP_MersennsMultMod(t2, y2, t2);         // t2=t2*y2

    ota_MP_SubMod(t1, t1, x1);              // t1=t1-x1
    ota_MP_SubMod(t2, t2, y1);              // t2=t2-y1


    if(ota_MP_isZero(t1))
    {
        if(ota_MP_isZero(t2))
        {
            ota_ECC_Double(r, q) ;
            return;
        }
        else
        {
            ota_MP_Init(z3);
            return;             // return infinity
        }
    }

    ota_MP_MersennsMultMod(z3, z1, t1);                 // z3=z1*t1
    ota_MP_MersennsSquaMod(s, t1);                      // t3=t1^2
    ota_MP_MersennsMultMod(k, s, t1);                   // t4=t3*t1
    ota_MP_MersennsMultMod(s, s, x1);                   // t3=t3*x1
    ota_MP_LShiftMod(t1, s);                    // t1=2*t3
    ota_MP_MersennsSquaMod(x3, t2);                     // x3=t2^2
    ota_MP_SubMod(x3, x3, t1);                      // x3=x3-t1
    ota_MP_SubMod(x3, x3, k);                       // x3=x3-t4
    ota_MP_SubMod(s, s, x3);                        // t3=t3-x3
    ota_MP_MersennsMultMod(s, s, t2);                   // t3=t3*t2
    ota_MP_MersennsMultMod(k, k, y1);                   // t4=t4*t1
    ota_MP_SubMod(y3, s, k);
}

// ECDSA Verify
BOOL32 ota_ecdsa_verify(unsigned char* digest, unsigned char* signature, Point* key)
{
   UINT32 e[KEY_LENGTH_DWORDS];
   UINT32 r[KEY_LENGTH_DWORDS];
   UINT32 s[KEY_LENGTH_DWORDS];

   UINT32 u1[KEY_LENGTH_DWORDS];
   UINT32 u2[KEY_LENGTH_DWORDS];

   UINT32 tmp1[KEY_LENGTH_DWORDS];
   UINT32 tmp2[KEY_LENGTH_DWORDS];

   Point p1, p2;
   UINT32 i;

   /* init for Coverity issue470141 */
   memset( &p1, 0x00, sizeof(p1) );

      // swap input data endianess
   for(i = 0; i < KEY_LENGTH_DWORDS; i++)
   {
       // import digest to long integer
       e[KEY_LENGTH_DWORDS-1-i] = BE_SWAP(digest, 4*i);

       // import signature to long integer
       r[KEY_LENGTH_DWORDS-1-i] = BE_SWAP(signature, 4*i);
       s[KEY_LENGTH_DWORDS-1-i] = BE_SWAP(signature, KEY_LENGTH_BYTES+4*i);
   }

   // compute s' = s ^ -1 mod n
   ota_MP_InvMod(tmp1, s, modn);

   // convert s' to montgomery domain
   ota_MP_MultMont(tmp2, tmp1, (DWORD*)ota_rr);

   // convert e to montgomery domain
   ota_MP_MultMont(tmp1, e, (DWORD*)ota_rr);

   // compute u1 = e * s' mod n
   ota_MP_MultMont(u1, tmp1, tmp2);

   // convert r to montgomery domain
   ota_MP_MultMont(tmp1, r, (DWORD*)ota_rr);

   // compute u2 = r * s' (mod n)
   ota_MP_MultMont(u2, tmp1, tmp2);

   // set tmp1 = 1
   ota_MP_Init(tmp1);
   tmp1[0]=1;

   // convert u1 to normal domain
   ota_MP_MultMont(u1, u1, tmp1);

   // convert u2 to normal domain
   ota_MP_MultMont(u2, u2, tmp1);

   // compute (x,y) = u1G + u2QA
   if(key)
   {
       // if public key is given, using legacy method
       ota_ECC_PM_B_NAF(&p1, &(ota_curve.G), u1);
       ota_ECC_Add(&p1, &p1, (PointAff*)&p2);

       // convert point to affine domain
       ota_MP_InvMod(tmp1, p1.z, modp);
       ota_MP_MersennsSquaMod(p1.z, tmp1);
       ota_MP_MersennsMultMod(p1.x, p1.x, p1.z);
   }
   else
   {
       printf("Error\n");
#if 0
       // if public key is not given, using pre-computed method
       ecdsa_fp_shamir(&p1, u1, u2);
#endif
   }

   // w = r (mod n)
   if(ota_MP_CMP(r, modp) >= 0)
   {
       ota_MP_Sub(r, r, modp);
   }

   // verify r == x ?
   if(memcmp(r, p1.x, KEY_LENGTH_BYTES))
       return FALSE;

   // signature match, return true
   return TRUE;
}
