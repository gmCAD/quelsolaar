#if !defined ENOUGH_DATA_TYPES
#define ENOUGH_DATA_TYPES


#define E_GEOMETRY_REAL_PRESISSION_64_BIT
/*#define E_GEOMETRY_REAL_PRESISSION_32_BIT*/

#ifdef E_GEOMETRY_REAL_PRESISSION_64_BIT
typedef double egreal;
#define E_GEOMETRY_SUBSCRIBE VN_FORMAT_REAL64
#define E_REAL_MAX V_REAL64_MAX
#endif

#ifdef E_GEOMETRY_REAL_PRESISSION_32_BIT
typedef float egreal;
#define E_GEOMETRY_SUBSCRIBE VN_FORMAT_REAL32
#define E_REAL_MAX V_REAL32_MAX
#endif

/*#define E_BITMAP_REAL_PRESISSION_64_BIT*/
#define E_BITMAP_REAL_PRESISSION_32_BIT

#ifdef E_BITMAP_REAL_PRESISSION_64_BIT
typedef double ebreal;
#endif

#ifdef E_BITMAP_REAL_PRESISSION_32_BIT
typedef float ebreal;
#endif

#endif
