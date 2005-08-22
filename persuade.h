#if !defined PERSUADE_H
#define	PERSUADE_H

#include "enough.h"



typedef void PMesh;


extern void persuade_init(uint max_tesselation_level, void *(*gl_GetProcAddress)(const char* proc)); /* initialize persuade and set the maximum SDS level*/
extern void p_task_compute(uint count); /* this function makes Persuade update everything that needs to be computedcompute */

/*renderable meshes*/

extern PMesh	*p_rm_create(ENode *node); /* use to create a drawabel version of a geometry node */
extern void		p_rm_destroy(PMesh *mesh); /* Destroy the same*/

extern PMesh		*p_rm_service(PMesh *mesh, ENode *o_node, const egreal *vertex); /* service the mesh to make it drawabel */
extern void		p_rm_compute(PMesh *mesh, const egreal *vertex);
extern void		p_rm_set_eay(PMesh *mesh, egreal *eay); /* set a view point if you want view specific LOD */
extern boolean	p_rm_validate(PMesh *mesh); /* is the mesh in sync with the geometry node */
extern boolean	p_rm_drawable(PMesh *mesh); /* is the mesh drawable */

extern egreal	*p_rm_get_vertex(PMesh *mesh); /* get the array of vertices to draw */
extern egreal	*p_rm_get_normal(PMesh *mesh); /* get the array of normals to draw */
extern uint		p_rm_get_vertex_length(PMesh *mesh); /* get the length of the vertex array */
extern uint		*p_rm_get_reference(PMesh *mesh); /* get the reference array of the object */
extern uint		p_rm_get_ref_length(PMesh *mesh); /* get the length of the reference array */

void *p_rm_get_param(PMesh *mesh);

extern egreal	p_rm_compute_bounding_box(ENode *node, egreal *vertex, egreal *center, egreal *scale);

/* Texture handler */

typedef void PTextureHandle;

extern void		p_th_texture_restart(void);
extern PTextureHandle *p_th_create_texture_handle(uint node_id, char *layer_r, char *layer_g, char *layer_b);
extern void		p_th_destroy_texture_handle(PTextureHandle *handle);
extern uint		p_th_get_texture_id(PTextureHandle *handle);

extern void		p_draw_scene(void);

#endif
