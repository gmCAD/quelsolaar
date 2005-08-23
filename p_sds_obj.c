
#include "enough.h"

#include "seduce.h"

#include "p_sds_geo.h"
#include "p_sds_table.h"
#include "p_sds_obj.h"
#include "p_task.h"

#include <math.h>

extern void p_get_tri_tess_index(uint *index, uint base_tess);
extern void p_get_quad_tess_index(uint *index, uint base_tess);

PMesh *p_rm_create(ENode *node)
{
	PMesh *mesh;
	if(e_ns_get_node_type(node) != V_NT_GEOMETRY)
		return NULL;
	mesh = malloc(sizeof *mesh);
	mesh->geometry_id =	e_ns_get_node_id(node);
	mesh->geometry_version = e_ns_get_node_version_struct(node);
	mesh->stage = 0;
	mesh->sub_stages[0] = 0;
	mesh->sub_stages[1] = 0;
	mesh->sub_stages[2] = 0;
	mesh->sub_stages[3] = 0;
	mesh->tess.force = 0;
	mesh->tess.factor = 50;
	mesh->tess.edge_tess_func = p_sds_edge_tesselation_global_func;
	mesh->temp = NULL;
	mesh->next = NULL;
	mesh->tess.tess = NULL;
	mesh->tess.order_node = NULL;
	mesh->tess.order_temp_mesh = NULL;
	mesh->tess.order_temp_mesh_rev = NULL;
	mesh->tess.order_temp_poly_start = NULL;
	mesh->render.vertex_array = NULL;
	mesh->render.normal_array = NULL;
	mesh->render.reference = NULL;
	mesh->render.mat = NULL;
	mesh->depend.reference = NULL;
	mesh->depend.weight = NULL;
	mesh->depend.ref_count = NULL;
	mesh->normal.normal_ref = NULL;	
	mesh->normal.normals = NULL;
	mesh->displacement.displacement = NULL;
	mesh->displacement.live = TRUE;
	mesh->displacement.node_version = 0;
	mesh->displacement.tree_version = 0;
	mesh->normal.draw_normals = NULL;
	mesh->param.array = NULL;
	mesh->param.version = NULL;
	mesh->anim.layers.layers = NULL;
	mesh->anim.bones.bonereference = NULL;
	mesh->anim.bones.ref_count = NULL;
	mesh->anim.bones.boneweight = NULL;
	mesh->anim.cvs = NULL; 
	return mesh;
}

void p_rm_set_eay(PMesh *mesh, egreal *eay)
{
	mesh->tess.eay[0] = eay[0];
	mesh->tess.eay[1] = eay[1];
	mesh->tess.eay[2] = eay[2];
	mesh->tess.edge_tess_func = p_sds_edge_tesselation_global_func;
}


void p_rm_destroy(PMesh *mesh)
{
	if(mesh->temp != NULL)
		free(mesh->temp);
	if(mesh->tess.tess != NULL)
		free(mesh->tess.tess);
	if(mesh->tess.order_node != NULL)
		free(mesh->tess.order_node);
	if(mesh->tess.order_temp_mesh != NULL)
		free(mesh->tess.order_temp_mesh);
	if(mesh->tess.order_temp_mesh_rev != NULL)
		free(mesh->tess.order_temp_mesh_rev);
	if(mesh->tess.order_temp_poly_start != NULL)
		free(mesh->tess.order_temp_poly_start);
	if(mesh->render.vertex_array != NULL)
		free(mesh->render.vertex_array);
	if(mesh->render.normal_array != NULL)
		free(mesh->render.normal_array);
	if(mesh->render.reference != NULL)
		free(mesh->render.reference);
	if(mesh->render.mat != NULL)
		free(mesh->render.mat);
	if(mesh->depend.reference != NULL)
		free(mesh->depend.reference);
	if(mesh->depend.weight != NULL)
		free(mesh->depend.weight);
	if(mesh->depend.ref_count != NULL)
		free(mesh->depend.ref_count);
	if(mesh->normal.normal_ref != NULL)
		free(mesh->normal.normal_ref);	
	if(mesh->normal.normals != NULL)
		free(mesh->normal.normals);
	if(mesh->displacement.displacement != NULL)
		free(mesh->displacement.displacement);
	if(mesh->normal.draw_normals != NULL)
		free(mesh->normal.draw_normals);
	if(mesh->param.array != NULL)
		free(mesh->param.array);
	if(mesh->param.version != NULL)
		free(mesh->param.version);
	if(mesh->anim.layers.layers != NULL)
		free(mesh->anim.layers.layers);
	if(mesh->anim.bones.bonereference != NULL)
		free(mesh->anim.bones.bonereference);
	if(mesh->anim.bones.ref_count != NULL)
		free(mesh->anim.bones.ref_count);
	if(mesh->anim.bones.boneweight != NULL)
		free(mesh->anim.bones.boneweight);
	if(mesh->anim.cvs != NULL)
		free(mesh->anim.cvs); 
	free(mesh);
}

boolean p_rm_validate(PMesh *mesh)
{
	PPolyStore *smesh;
	smesh = p_sds_get_mesh(e_ns_get_node(0, mesh->geometry_id));
	if(smesh == NULL || mesh->geometry_version != smesh->version)
		return FALSE;
	return TRUE;
}


typedef enum{
	POS_ALLOCATE,
	POS_FIND_HOLES_MAT_SORT,
	POS_TESS_SELECT,
	POS_SECOND_ALLOCATE,
	POS_CREATE_REF,
	POS_CREATE_DEPEND,
	POS_CREATE_VERTEX_NORMAL,
	POS_CREATE_NORMAL_REF,
	POS_CREATE_LAYER_PARAMS,
	POS_CREATE_DISPLACEMENT,
	POS_CREATE_ANIM,
	POS_ANIMATE,
	POS_CREATE_VERTICES,
	POS_DONE
}PObjStage;

boolean p_rm_drawable(PMesh *mesh)
{
	return mesh->stage == POS_DONE;
}

void p_rm_unvalidate(PMesh *mesh)
{
	mesh->geometry_version--;
}

uint p_rm_get_geometry_node(PMesh *mesh)
{
	return mesh->geometry_id;
}

void *p_rm_get_param(PMesh *mesh)
{
	return mesh->param.array;
}

uint p_rm_get_param_count(PMesh *mesh)
{
	return mesh->param.array_count;
}


#define P_TRI_TESS_SELECT 10000
#define P_QUAD_TESS_SELECT 7500
#define P_TRI_TESS_REF 10000
#define P_QUAD_TESS_REF 7500



PMesh *p_rm_service(PMesh *mesh, ENode *o_node, /*const*/ egreal *vertex)
{
	ENode *g_node;
	PMesh *store = NULL;
	PTessTableElement *table;
	PPolyStore *smesh;
	uint i, j, k, poly_size = 4;
	uint32 seconds, fractions;

	g_node = e_ns_get_node(0, mesh->geometry_id);
	smesh = p_sds_get_mesh(g_node);

	if(vertex == NULL)
		vertex = e_nsg_get_layer_data(g_node, e_nsg_get_layer_by_id(g_node, 0));
	if(smesh == NULL || e_ns_get_node_version_struct(g_node) != smesh->geometry_version)
		return mesh;

	verse_session_get_time(&seconds, &fractions);
	if(mesh->stage == POS_DONE && mesh->next == NULL && (p_lod_compute_lod_update(o_node, g_node, seconds, fractions, mesh->tess.factor) || p_lod_material_test(mesh, o_node)))
		mesh->geometry_version--;



	if(smesh->geometry_version != mesh->geometry_version && mesh->next == NULL)
	{
		if(mesh->stage != POS_DONE)
		{
			p_rm_destroy(mesh);
			return p_rm_create(g_node);
		}else
		{
			mesh->next = p_rm_create(g_node);
		}
	}
	if(mesh->next != NULL)
	{
		if(smesh->geometry_version != ((PMesh *)mesh->next)->geometry_version)
		{
			p_rm_destroy(mesh->next);
			mesh->next = NULL;
			return mesh;
		}
		store = mesh;
		mesh = mesh->next;
	}
	for(i = 1 ; i < smesh->level; i++)
		poly_size *= 4;

	switch(mesh->stage)
	{
		case POS_ALLOCATE : /* clearing and allocating */
			mesh->tess.tri_count = smesh->base_tri_count;
			mesh->tess.quad_count = smesh->base_quad_count;
			mesh->tess.tess = malloc((sizeof *mesh->tess.tess) * (mesh->tess.tri_count + mesh->tess.quad_count));
			mesh->tess.factor = p_lod_compute_lod_level(o_node, g_node, seconds, fractions);
			mesh->temp = NULL;
			mesh->render.element_count = 0;
			mesh->render.vertex_count = 0;
			mesh->depend.length = 0;
			mesh->anim.cv_count = e_nsg_get_vertex_length(g_node);
			mesh->anim.cvs = malloc((sizeof *mesh->anim.cvs) * mesh->anim.cv_count * 3); 
			mesh->anim.play.layers = TRUE;
			mesh->anim.play.bones = TRUE;
			mesh->anim.layers.data_version = 0;
			mesh->anim.layers.layers = NULL;
			mesh->anim.layers.layer_count = 0;
			mesh->anim.bones.bonereference = NULL;
			mesh->anim.bones.ref_count = NULL;
			mesh->anim.bones.boneweight = NULL;
			mesh->anim.scale[0] = 0;
			mesh->anim.scale[1] = 0;
			mesh->anim.scale[2] = 0;
			mesh->sub_stages[0] = 0;
			mesh->sub_stages[1] = 0;
			mesh->sub_stages[2] = 0;
			mesh->sub_stages[3] = 0;
			mesh->stage++;
			break;
		case POS_FIND_HOLES_MAT_SORT : /* handle_materials */
			p_lod_gap_count(g_node, smesh, mesh, o_node);
			break;
		case POS_TESS_SELECT : /* choosing quad split tesselation */
			p_lod_select_tesselation(mesh, smesh, vertex);
			break;
		case POS_SECOND_ALLOCATE : /* allocating all the stuff */
			mesh->render.vertex_array = malloc((sizeof *mesh->render.vertex_array) * mesh->render.vertex_count * 3);
			mesh->render.normal_array = malloc((sizeof *mesh->render.normal_array) * mesh->render.vertex_count * 3);
			mesh->normal.normal_ref = malloc((sizeof *mesh->normal.normal_ref) * mesh->render.vertex_count * 4);
			mesh->render.reference = malloc((sizeof *mesh->render.reference) * mesh->render.element_count * 3);

			mesh->displacement.displacement = NULL;

			mesh->depend.reference = malloc((sizeof *mesh->depend.reference) * mesh->depend.length);
			mesh->depend.weight = malloc((sizeof *mesh->depend.weight) * mesh->depend.length);
			mesh->depend.ref_count = malloc((sizeof *mesh->depend.ref_count) * mesh->render.vertex_count);

			mesh->render.vertex_count = 0;
			mesh->depend.length = 0;
			mesh->stage++;
			break;
		case POS_CREATE_REF : /* building reference */
			for(; mesh->sub_stages[0] < mesh->tess.tri_count + mesh->tess.quad_count; mesh->sub_stages[0]++)
			{
				if(mesh->sub_stages[0] == mesh->render.mat[mesh->sub_stages[3]].tri_end)
				{
					mesh->render.mat[mesh->sub_stages[3]].render_end = mesh->sub_stages[2];
					mesh->sub_stages[3]++;
				}
				table = mesh->tess.tess[mesh->sub_stages[0]];
				for(j = 0; j < table->element_count;)
				{
					mesh->render.reference[mesh->sub_stages[2]++] = table->index[j++] + mesh->sub_stages[1];
					mesh->render.reference[mesh->sub_stages[2]++] = table->index[j++] + mesh->sub_stages[1];
					mesh->render.reference[mesh->sub_stages[2]++] = table->index[j++] + mesh->sub_stages[1];
				}
				mesh->sub_stages[1] += table->vertex_count;
			}
			if(mesh->sub_stages[0] == mesh->tess.tri_count + mesh->tess.quad_count)
			{
				mesh->render.mat[mesh->sub_stages[3]].render_end = mesh->sub_stages[2];
				mesh->stage++;
				mesh->sub_stages[0] = 0;
				mesh->sub_stages[1] = 0;
				mesh->sub_stages[2] = 0;
				mesh->sub_stages[3] = 0;
			}
			break;
		case POS_CREATE_DEPEND : /* building depend */
			{
				PDepend *dep;
				uint poly;
				for(; mesh->sub_stages[0] < mesh->tess.tri_count + mesh->tess.quad_count; mesh->sub_stages[0]++)
				{
					table = mesh->tess.tess[mesh->sub_stages[0]];

					if(mesh->sub_stages[0] == mesh->render.mat[mesh->sub_stages[1]].tri_end)
						mesh->sub_stages[1]++;
					if(mesh->sub_stages[0] < mesh->render.mat[mesh->sub_stages[1]].quad_end)
						poly = mesh->sub_stages[0] * smesh->poly_per_base * 4;
					else
						poly = smesh->quad_length / 4 + mesh->sub_stages[0] * smesh->poly_per_base * 3;

					for(j = 0; j < table->vertex_count; j++)
					{
						
						dep = &smesh->vertex_dependency[smesh->ref[table->reference[j] + poly]];
						mesh->depend.ref_count[mesh->render.vertex_count++] = dep->length;
						for(k = 0; k < dep->length; k++)
						{
							mesh->depend.reference[mesh->depend.length] = dep->element[k].vertex * 3;
							mesh->depend.weight[mesh->depend.length] = dep->element[k].value;
							mesh->depend.length++;
						}
					}
					mesh->tess.order_temp_poly_start[mesh->sub_stages[0]] = mesh->sub_stages[2];
					mesh->sub_stages[2] += table->vertex_count;
				}
				if(mesh->sub_stages[0] == mesh->tess.tri_count + mesh->tess.quad_count)
				{
					mesh->stage++;
					mesh->sub_stages[0] = 0;
					mesh->sub_stages[1] = 0;
					mesh->sub_stages[2] = 0;
					mesh->sub_stages[3] = 0;
				}
				break;
			}
		case POS_CREATE_VERTEX_NORMAL :
			p_lod_compute_vertex_normals(smesh, mesh);
			mesh->stage++;
			break;
		case POS_CREATE_NORMAL_REF :
			p_lod_create_normal_ref_and_shadow_skirts(smesh, mesh);
			break;
		case POS_CREATE_LAYER_PARAMS :
			p_lod_create_layer_param(g_node, mesh);
			break;
		case POS_CREATE_DISPLACEMENT :
			if(o_node != NULL)	
			{
				if(p_lod_displacement_update_test(mesh))
				{
					uint ii;
					mesh->displacement.displacement = malloc((sizeof *mesh->displacement.displacement) * mesh->render.vertex_count);
					p_lod_create_displacement_array(g_node, o_node, mesh, smesh->level);
				//	for(ii = 0; ii < mesh->render.vertex_count; ii++)
				//		mesh->displacement.displacement[ii] = 0;
				}
			}
			else
				mesh->stage++;
			break;
		case POS_CREATE_ANIM :
			if(o_node != NULL)	
			{
				p_lod_anim_bones_update_test(mesh, o_node, g_node);
				p_lod_anim_scale_update_test(mesh, o_node);
				p_lod_anim_layer_update_test(mesh, o_node, g_node);
			}
			mesh->stage++;
			break;
		case POS_ANIMATE :
			if(o_node != NULL)	
				p_lod_anim_vertex_array(mesh->anim.cvs, mesh->anim.cv_count, mesh, g_node);
			mesh->stage++;
			break;
		case POS_CREATE_VERTICES :
			if(o_node != NULL)	
				p_lod_compute_vertex_array(mesh->render.vertex_array, mesh->render.vertex_count, mesh->depend.ref_count, mesh->depend.reference, mesh->depend.weight, mesh->anim.cvs);
			else
				p_lod_compute_vertex_array(mesh->render.vertex_array, mesh->render.vertex_count, mesh->depend.ref_count, mesh->depend.reference, mesh->depend.weight, vertex);
			p_lod_compute_normal_array(mesh->render.normal_array, mesh->render.vertex_count, mesh->normal.normal_ref, mesh->render.vertex_array);
		//	if(o_node != NULL)	
		//		p_lod_create_displacement_array(g_node, o_node, mesh, smesh->level);
			if(mesh->displacement.displacement != NULL)
			{
				p_lod_compute_displacement_array(mesh->render.vertex_array, mesh->render.vertex_count, mesh->render.normal_array, mesh->displacement.displacement);
				p_lod_compute_normal_array(mesh->render.normal_array, mesh->render.vertex_count, mesh->normal.normal_ref, mesh->render.vertex_array);
			}
			mesh->stage++;
			if(store != NULL)
				p_rm_destroy(store);
			store = NULL;
			break;
		case POS_DONE :
			if(o_node != NULL)
			{
				boolean update = FALSE;
				static double timer = 0;
				timer += 0.1;
				p_lod_update_layer_param(g_node, mesh);
				if(p_lod_anim_bones_update_test(mesh, o_node, g_node))
					update = TRUE;
				if(p_lod_anim_scale_update_test(mesh, o_node))
					update = TRUE;
				if(p_lod_anim_layer_update_test(mesh, o_node, g_node))
					update = TRUE;
				if(p_lod_displacement_update_test(mesh))
				{
					uint ii;
					p_lod_update_displacement_array(g_node, o_node, mesh, smesh->level);
					update = TRUE;
				}
				if(update)
				{
					p_lod_anim_vertex_array(mesh->anim.cvs, mesh->anim.cv_count, mesh, g_node);
					p_lod_compute_vertex_array(mesh->render.vertex_array, mesh->render.vertex_count, mesh->depend.ref_count, mesh->depend.reference, mesh->depend.weight, mesh->anim.cvs);
					p_lod_compute_normal_array(mesh->render.normal_array, mesh->render.vertex_count, mesh->normal.normal_ref, mesh->render.vertex_array);
					if(mesh->displacement.displacement != NULL)
					{
						p_lod_compute_displacement_array(mesh->render.vertex_array, mesh->render.vertex_count, mesh->render.normal_array, mesh->displacement.displacement);
						p_lod_compute_normal_array(mesh->render.normal_array, mesh->render.vertex_count, mesh->normal.normal_ref, mesh->render.vertex_array);
					}
				}

			}
			break;
	}
	if(store != NULL)
		return store;
	return mesh;
}

void p_rm_update_shape(PMesh *mesh, egreal *vertex)
{
	p_lod_compute_vertex_array(mesh->render.vertex_array, mesh->render.vertex_count, mesh->depend.ref_count, mesh->depend.reference, mesh->depend.weight, vertex);
	p_lod_compute_normal_array(mesh->render.normal_array, mesh->render.vertex_count, mesh->normal.normal_ref, mesh->render.vertex_array);
}


/*
void p_rm_compute(PMesh *mesh, egreal *vertex, uint start, uint length)
{
	uint i, j, k = 0, l, count, *ref;
	egreal f;
	ref = mesh->depend.reference;

	for(i = 0; i < mesh->render.vertex_count; i++)
	{
		mesh->render.vertex_array[i * 3] = 0;
		mesh->render.vertex_array[i * 3 + 1] = 0;
		mesh->render.vertex_array[i * 3 + 2] = 0;
		count = mesh->depend.ref_count[i];
		for(j = 0; j < count; j++)
		{
			l = mesh->depend.reference[k];
			f = mesh->depend.weight[k];
			mesh->render.vertex_array[i * 3] += vertex[l++] * f;
			mesh->render.vertex_array[i * 3 + 1] += vertex[l++] * f;
			mesh->render.vertex_array[i * 3 + 2] += vertex[l] * f;
			k++;
		}
	}
}
*/

boolean p_rm_draw_ready(PMesh *mesh)
{
	return mesh->stage > 7;
}

#define NORMAL_ADD 0.001

void p_lod_compute_vertex_array(egreal *vertex, uint vertex_count, const uint *ref_count, const uint *reference,  const egreal *weight, const egreal *cvs)
{
	uint i, j, k = 0, v = 0, count, r;
	egreal f;
	for(i = 0; i < vertex_count; i++)
	{
		vertex[v + 0] = 0;
		vertex[v + 1] = 0;
		vertex[v + 2] = 0;
		count = ref_count[i];
		for(j = 0; j < count; j++)
		{
			r = reference[k];
			f = weight[k++];
			vertex[v + 0] += cvs[r++] * f;
			vertex[v + 1] += cvs[r++] * f;
			vertex[v + 2] += cvs[r] * f;
		}
//		printf("%f %f %f\n", vertex[v + 0], vertex[v + 1], vertex[v + 2]);
		v += 3;
	}

}

void p_lod_compute_displacement_array(egreal *vertex, uint vertex_count, const egreal *normals, const egreal *displacement)
{
	uint i;
	for(i = 0; i < vertex_count; i++)
	{
		*vertex++ += *normals++ * *displacement * 100;
		*vertex++ += *normals++ * *displacement * 100;
		*vertex++ += *normals++ * *displacement++ * 100;
	}
}

void p_lod_compute_normal_array(egreal *normals, uint vertex_count, const uint *normal_ref, const egreal *vertex)
{
	uint i = 0, j = 0;
	egreal x, y, z, vec0[3], vec1[3], vec2[3], vec3[3];
	vertex_count *= 3;
	while(i < vertex_count)
	{
		x = vertex[i];
		y = vertex[i + 1];
		z = vertex[i + 2];

		vec0[0] = vertex[normal_ref[j] * 3] - x;
		vec0[1] = vertex[normal_ref[j] * 3 + 1] - y;
		vec0[2] = vertex[normal_ref[j] * 3 + 2] - z;
		j++;
		vec1[0] = vertex[normal_ref[j] * 3] - x;
		vec1[1] = vertex[normal_ref[j] * 3 + 1] - y;
		vec1[2] = vertex[normal_ref[j] * 3 + 2] - z;
		j++;
		vec2[0] = vertex[normal_ref[j] * 3] - x;
		vec2[1] = vertex[normal_ref[j] * 3 + 1] - y;
		vec2[2] = vertex[normal_ref[j] * 3 + 2] - z;
		j++;
		vec3[0] = vertex[normal_ref[j] * 3] - x;
		vec3[1] = vertex[normal_ref[j] * 3 + 1] - y;
		vec3[2] = vertex[normal_ref[j] * 3 + 2] - z;
		j++;

		normals[i++] = (vec0[1] * vec1[2] - vec0[2] * vec1[1]) + (vec2[1] * vec3[2] - vec2[2] * vec3[1]);
		normals[i++] = (vec0[2] * vec1[0] - vec0[0] * vec1[2]) + (vec2[2] * vec3[0] - vec2[0] * vec3[2]);
		normals[i++] = (vec0[0] * vec1[1] - vec0[1] * vec1[0]) + (vec2[0] * vec3[1] - vec2[1] * vec3[0]);

		{
			egreal a;
			a = sqrt(normals[i - 3] * normals[i - 3] + normals[i - 2] * normals[i - 2] + normals[i - 1] * normals[i - 1]);
			normals[i - 1] /= a;
			normals[i - 2] /= a;
			normals[i - 3] /= a;
		}
	}
}


egreal *p_rm_get_vertex(PMesh *mesh)
{
	return mesh->render.vertex_array;
}

egreal *p_rm_get_normal(PMesh *mesh)
{
	return mesh->render.normal_array;
}

uint p_rm_get_vertex_length(PMesh *mesh)
{
	return mesh->render.vertex_count;
}

uint *p_rm_get_reference(PMesh *mesh)
{
	return mesh->render.reference;
}

uint p_rm_get_ref_length(PMesh *mesh)
{
	return mesh->render.element_count;
}

uint p_rm_get_mat_count(PMesh *mesh)
{
	return mesh->render.mat_count;
}

uint p_rm_get_material_range(PMesh *mesh, uint mat)
{
	return mesh->render.mat[mat].render_end;
}

uint p_rm_get_material(PMesh *mesh, uint mat)
{
	return mesh->render.mat[mat].material;
}