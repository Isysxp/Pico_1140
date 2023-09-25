#pragma once
#include <stdint.h>

// interrupts
enum INTVEC {
    INTBUS = 0004,
    INTINVAL = 0010,
    INTDEBUG = 0014,
    INTIOT = 0020,
    INTTTYIN = 0060,
    INTTTYOUT = 0064,
    INTFAULT = 0250,
    INTCLOCK = 0100,
    INTRK = 0220,
    INTFIS = 0244,
    INTRL = 0160,
    INTDLR = 0300,
    INTDLT = 0304,
    INTFPP = 0244
};

[[ noreturn ]] void trap(uint16_t num);



#if defined (VMS)
#include <ints.h>
#elif defined(_MSC_VER) && (_MSC_VER < 1600)
typedef __int8           int8;
typedef __int16          int16;
typedef __int32          int32;
typedef unsigned __int8  uint8;
typedef unsigned __int16 uint16;
typedef unsigned __int32 uint32;
#else                                                   
/* All modern/standard compiler environments */
/* any other environment needa a special case above */
#include <stdint.h>
typedef int8_t          int8;
typedef int16_t         int16;
typedef int32_t         int32;
typedef uint8_t         uint8;
typedef uint16_t        uint16;
typedef uint32_t        uint32;
#endif                                                  /* end standard integers */
typedef int t_bool;

/* Floating point accumulators */

typedef struct {
    uint32              l;                              /* low 32b */
    uint32              h;                              /* high 32b */
} fpac_t;

/* PSW */

#define PSW_V_C         0                               /* condition codes */
#define PSW_V_V         1
#define PSW_V_Z         2
#define PSW_V_N         3
#define PSW_V_TBIT      4                               /* trace trap */
#define PSW_V_IPL       5                               /* int priority */
#define PSW_V_FPD       8                               /* first part done */
#define PSW_V_RS        11                              /* register set */
#define PSW_V_PM        12                              /* previous mode */
#define PSW_V_CM        14                              /* current mode */
#define PSW_CC          017
#define PSW_TBIT        (1 << PSW_V_TBIT)
#define PSW_PM          (3 << PSW_V_PM)

/* FPS */

#define FPS_V_C         0                               /* condition codes */
#define FPS_V_V         1
#define FPS_V_Z         2
#define FPS_V_N         3
#define FPS_V_T         5                               /* truncate */
#define FPS_V_L         6                               /* long */
#define FPS_V_D         7                               /* double */
#define FPS_V_IC        8                               /* ic err int */
#define FPS_V_IV        9                               /* overflo err int */
#define FPS_V_IU        10                              /* underflo err int */
#define FPS_V_IUV       11                              /* undef var err int */
#define FPS_V_ID        14                              /* int disable */
#define FPS_V_ER        15                              /* error */


/* Protection modes */

#define MD_KER          0
#define MD_SUP          1
#define MD_UND          2
#define MD_USR          3

/* Architectural constants */

#define STKL_R          0340                            /* stack limit */
#define STKL_Y          0400




