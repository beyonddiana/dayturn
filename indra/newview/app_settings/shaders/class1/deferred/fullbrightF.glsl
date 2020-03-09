/** 
 * @file fullbrightF.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */
 
#extension GL_ARB_texture_rectangle : enable

/*[EXTRA_CODE_HERE]*/

#ifdef DEFINE_GL_FRAGCOLOR
out vec4 frag_color;
#else
#define frag_color gl_FragColor
#endif

#if !HAS_DIFFUSE_LOOKUP
uniform sampler2D diffuseMap;
#endif

VARYING vec3 vary_position;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

#ifdef WATER_FOG
vec4 applyWaterFogView(vec3 pos, vec4 color);
#endif

vec3 fullbrightAtmosTransport(vec3 light);
vec3 fullbrightScaleSoftClip(vec3 light);

vec3 srgb_to_linear(vec3 cs)
{
	vec3 low_range = cs / vec3(12.92);
	vec3 high_range = pow((cs+vec3(0.055))/vec3(1.055), vec3(2.4));
	bvec3 lte = lessThanEqual(cs,vec3(0.04045));

#ifdef OLD_SELECT
	vec3 result;
	result.r = lte.r ? low_range.r : high_range.r;
	result.g = lte.g ? low_range.g : high_range.g;
	result.b = lte.b ? low_range.b : high_range.b;
    return result;
#else
	return mix(high_range, low_range, lte);
#endif

}

vec3 linear_to_srgb(vec3 cl)
{
	cl = clamp(cl, vec3(0), vec3(1));
	vec3 low_range  = cl * 12.92;
	vec3 high_range = 1.055 * pow(cl, vec3(0.41666)) - 0.055;
	bvec3 lt = lessThan(cl,vec3(0.0031308));

#ifdef OLD_SELECT
	vec3 result;
	result.r = lt.r ? low_range.r : high_range.r;
	result.g = lt.g ? low_range.g : high_range.g;
	result.b = lt.b ? low_range.b : high_range.b;
    return result;
#else
	return mix(high_range, low_range, lt);
#endif

}

#ifdef HAS_ALPHA_MASK
uniform float minimum_alpha;
#endif

void main() 
{
#if HAS_DIFFUSE_LOOKUP
	vec4 color = diffuseLookup(vary_texcoord0.xy);
#else
	vec4 color = texture2D(diffuseMap, vary_texcoord0.xy);
#endif

	float final_alpha = color.a * vertex_color.a;

#ifdef HAS_ALPHA_MASK
	if (color.a < minimum_alpha)
	{
		discard;
	}
#endif

	color.rgb *= vertex_color.rgb;
	color.rgb = fullbrightAtmosTransport(color.rgb);
	color.rgb = fullbrightScaleSoftClip(color.rgb);

#ifdef WATER_FOG
	vec3 pos = vary_position;
	vec4 fogged = applyWaterFogView(pos, vec4(color.rgb, final_alpha));
	color.rgb = fogged.rgb;
	color.a   = fogged.a;
#else
	color.a   = final_alpha;
#endif

	frag_color = color;
}

