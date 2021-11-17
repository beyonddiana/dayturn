 * @file moonF.glsl
 *
 * $LicenseInfo:firstyear=2005&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2005, Linden Research, Inc.
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

vec3 fullbrightAtmosTransport(vec3 light);
vec3 fullbrightScaleSoftClip(vec3 light);

uniform vec4 color;
uniform vec4 sunlight_color;
uniform vec3 lumWeights;
uniform float minLuminance;
uniform sampler2D diffuseMap;
VARYING vec2 vary_texcoord0;

void main() 
{
	vec4 c = texture2D(diffuseMap, vary_texcoord0.xy);
	c.rgb = fullbrightAtmosTransport(c.rgb);
    c.rgb = fullbrightScaleSoftClip(c.rgb);
    c.rgb = pow(c.rgb, vec3(0.45f));
    // mix factor which blends when sunlight is brighter
    // and shows true moon color at night
    float mix = dot(normalize(sunlight_color.rgb), lumWeights);
    mix = smoothstep(-0.5f, 2.0f, lum);
	frag_color = vec4(c.rgb, mix * c.a);
}
