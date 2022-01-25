/*

%%

pass Translucent
{
	vertex = main_VS
	pixel = main_PS

	render_state = {
		rasterizer = 	{
			fill_mode	= 'Solid'
			cull_mode 	= 'Back'
			flags = (
				'Enable_DepthClip'
			)
		}

		depth_stencil = 	{
			flags = (
				'Enable_DepthTest'
				; no depth writes
			)
			; Reverse Z
			comparison_function = 'Greater_Equal'
		}

		blend = {
			flags = (
				'Enable_Blending'
			)
			color_channel = {
				operation = 'ADD'
				source_factor = 'SRC_ALPHA'
				destination_factor = 'INV_SRC_ALPHA'
			}
			write_mask	= ('ENABLE_ALL')
		}
	}
}

%%
*/

#include <Shared/nw_shared_globals.h>
#include <Common/Colors.hlsl>
#include <Common/transform.hlsli>
#include "_PS.h"
#include <GBuffer/nw_gbuffer_read.h>	// DepthTexture
#include <GBuffer/nw_gbuffer_write.h>

/*
==========================================================
	STRUCTS
==========================================================
*/
struct VS_In
{
	float3	position: Position;
};

struct PS_In
{
	float4 position : SV_Position;
//	float3 color: Color;
};

/*
==========================================================
	VERTEX SHADER
==========================================================
*/
PS_In main_VS( in VS_In vs_in )
{
    PS_In vs_out;

	float4 local_position = float4(vs_in.position, 1.0f);
	vs_out.position = Pos_Local_To_Clip( local_position );

	return vs_out;
}

/*
==========================================================
	PIXEL SHADER
==========================================================
*/
void main_PS(
			 in PS_In ps_in,
			 out float4 color_out_: SV_Target
			 )
{
	color_out_ = GREEN;
	color_out_.a = 0.3;
}
