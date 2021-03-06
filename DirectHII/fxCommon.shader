
// -----------------------------------------------------------------------------------------------------------------------------------------------------------------
// CBUFFERS
// cBuffers are available to all shader stages and are divided by update frequency and usage

cbuffer cbDrawPerFrame : register(b0) {
	matrix orthoMatrix : packoffset(c0);
	float v_gamma : packoffset(c4.x);
	float v_contrast : packoffset(c4.y);
};

cbuffer cbMainPerFrame : register(b1) {
	matrix mvpMatrix : packoffset(c0);
	float3 viewOrigin : packoffset(c4);
	float3 viewForward : packoffset(c5);
	float3 viewRight : packoffset(c6);
	float3 viewUp : packoffset(c7);
	float2 skyScroll : packoffset(c8.x);
	float skyScale : packoffset(c8.z);
	float turbTime : packoffset(c8.w);
	float4 vBlend : packoffset(c9);
	float2 WarpTime : packoffset(c10.x);
};

cbuffer cbPerObject : register(b2) {
	matrix LocalMatrix : packoffset(c0);
	float AlphaVal : packoffset(c4.x);
	float AbsLight : packoffset(c4.y);
	float3 EntOrigin : packoffset(c5);
};

cbuffer cbPerMesh : register(b3) {
	float3 MeshScale : packoffset(c0);
	float3 MeshTranslate : packoffset(c1.x);
	float LerpBlend : packoffset(c1.w);
	float3 ShadeLight : packoffset(c2);
	float3 ShadeVector : packoffset(c3);
};

cbuffer cbPerLight : register(b4) {
	float3 LightOrigin : packoffset(c0.x);
	float LightRadius : packoffset(c0.w);
};


// common to mesh and surf
struct PS_DYNAMICLIGHT {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
	float3 LightVector : LIGHTVECTOR;
	float3 Normal : NORMAL;
};


static const float M_PI = 3.14159265f;


#ifdef VERTEXSHADER
struct VS_QUADBATCH {
	float4 Position : POSITION;
	float4 Color : COLOUR;
	float2 TexCoord : TEXCOORD;
};

Buffer<float> LightStyles : register(t0);
Buffer<float4> LightNormals : register(t1);
#endif


// this ensures that we use the same dynamic calcs on surfs and meshs
// outside of the #ifdef guard because it's used by the VS for meshs and the GS for surfs
PS_DYNAMICLIGHT GenericDynamicVS (float4 Position, float3 Normal, float2 TexCoord)
{
	PS_DYNAMICLIGHT vs_out;

	vs_out.Position = mul (LocalMatrix, Position);
	vs_out.TexCoord = TexCoord;
	vs_out.LightVector = LightOrigin - Position.xyz;
	vs_out.Normal = Normal;

	return vs_out;
}


#ifdef PIXELSHADER
// sampler slots
sampler mainSampler : register(s0);		// main sampler used for most objects, mig/mag point, mip linear, wrap mode, no anisotropy
sampler lmapSampler : register(s1);		// lightmap sampler, always linear, clamp mode, no mips, no anisotropy
sampler warpSampler : register(s2);		// underwater and other warps, always linear, wrap mode, no mips, no anisotropy
sampler drawSampler : register(s3);		// used for the 2d render; point sampled, wrap mode, no mips, no anisotropy

// texture slots
Texture2D<float4> mainTexture : register(t0);	// main diffuse texture on most objects
Texture2DArray<float4> lmapTexture : register(t1);	// lightmap styles 0/1/2/3
Texture2D<float4> solidSkyTexture : register(t2);	// solid sky layer
Texture2D<float4> alphaSkyTexture : register(t3);	// alpha sky layer
Texture2D<float4> warpTexture : register(t4);	// underwater warp noise texture


// faster than a full-screen gamma pass over the scene as a post-process, and more flexible than texture gamma
float4 GetGamma (float4 colorin)
{
	// gamma is not applied to alpha
	float3 contrasted = colorin.rgb * v_contrast;	// this isn't actually "contrast" but it's consistent with what other engines do
	return float4 (pow (max (contrasted, 0.0f), v_gamma), colorin.a);
}

// common to mesh and surf
float4 GenericDynamicPS (PS_DYNAMICLIGHT ps_in) : SV_TARGET0
{
	// this clip is sufficient to exclude unlit portions; Add below may still bring it to 0
	// but in practice it's rare and it runs faster without a second clip
	clip ((LightRadius * LightRadius) - dot (ps_in.LightVector, ps_in.LightVector));

	// reading the diffuse texture early so that it should interleave with some ALU ops
	float4 diff = GetGamma (mainTexture.Sample (mainSampler, ps_in.TexCoord));

	// this calc isn't correct per-theory but it matches with the calc used by light.exe and qrad.exe
	// at this stage we don't adjust for the overbright range; that will be done via the "intensity" cvar in the C code
	float Angle = ((dot (normalize (ps_in.Normal), normalize (ps_in.LightVector)) * 0.5f) + 0.5f) / 256.0f;

	// using our own custom attenuation, again it's not correct per-theory but matches the Quake tools
	float Add = max ((LightRadius - length (ps_in.LightVector)) * Angle, 0.0f);

	return float4 (diff.rgb * Add * AlphaVal, 0.0f);
}
#endif

