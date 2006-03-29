#include "st_matrix_operations.h"
#include "enough.h"
#include "seduce.h"
#include "deceive.h"

#include "co_vn_handle.h"

void co_pu_hide_all(void)
{

	ENode *node;
	COVerseNode *co_node;
	uint type;
	for(type = 0; type < V_NT_NUM_TYPES; type++)
	{
		for(node = e_ns_get_node_next(0, 0, type); node != NULL; node = e_ns_get_node_next(e_ns_get_node_id(node) + 1, 0, type))
		{
			co_node = e_ns_get_custom_data(node, CONNECTOR_ENOUGH_SLOT);
			co_node->hidden = TRUE;
		}
	}
}

extern boolean co_draw_3d_view_get(void);
extern void co_draw_3d_view_set(boolean set);

void co_pu_empty(BInputState *input)
{
	SUIPUElement element[4];
	static float x, y;
	uint ring;
	element[0].type = PU_T_ANGLE;
	element[0].text = "";//"Settings";
	element[0].data.angle[0] = -45;
	element[0].data.angle[1] = +45;

	element[1].type = PU_T_ANGLE;
	element[1].text = "Hide All";
	element[1].data.angle[0] = 45;
	element[1].data.angle[1] = 135;

	element[2].type = PU_T_ANGLE;
	element[2].text = "";//"Measure Grid";
	element[2].data.angle[0] = 135;
	element[2].data.angle[1] = 225;

	element[3].type = PU_T_ANGLE;
	if(co_draw_3d_view_get())
		element[3].text = "Hide scene";
	else
		element[3].text = "Show scene";
	element[3].data.angle[0] = 225;
	element[3].data.angle[1] = 315;

	if(input->mode == BAM_DRAW)
	{
		glDisable(GL_DEPTH_TEST);
		glPopMatrix();
		glPopMatrix();
		glPushMatrix();
		glTranslatef(0, 0, -1);
	}
	if(input->mouse_button[2] == TRUE && input->last_mouse_button[2] == FALSE)
	{
		x = input->pointer_x;
		y = input->pointer_y;
	}
	ring = sui_draw_popup(input, x, y, element, 4, 2, 1);
	switch(ring)
	{
		case 0 :
	/*		co_pu_hide_all();*/
		break;
		case 1 :
			co_pu_hide_all();
	/*		la_t_revolve(edge, 16);*/
		break;
		case 2 :
	/*		co_draw_3d_view_set(!co_draw_3d_view_get());*/
	/*		la_t_tube(edge, 8);*/
		break;
		case 3 :
			co_draw_3d_view_set(!co_draw_3d_view_get());
	/*		la_t_select_hull(edge);*/
		break;
	}
	if(input->mode == BAM_DRAW)
	{
		glPopMatrix();
		glPushMatrix();
		glPushMatrix();
		glEnable(GL_DEPTH_TEST);
	}
}