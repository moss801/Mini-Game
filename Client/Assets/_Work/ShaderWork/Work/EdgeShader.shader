Shader "Custom/NewSurfaceShader"
{
    Properties
    {
    	_ColorEdge ("ColorEdge", Color) = (1,0,0,1)
        _ColorTex ("ColorTexture", 2D) = "white" {}
    	_Width("Width",Range(0,0.3)) = 0.1
    }
    SubShader
    {
        Tags
        {
            "RenderPipeline" = "UniversalPipeline"
			"RenderType" = "Transparent" 
			"Queue" = "Transparent"
        }
        
        HLSLINCLUDE
		#include "Packages/com.unity.render-pipelines.universal/ShaderLibrary/Core.hlsl"
		ENDHLSL
        
	   Pass
       {
           Name "DrawEdgePass2"
           Tags{"LightMode" = "UniversalForward"}
           
           HLSLPROGRAM
           #pragma vertex Vert
           #pragma fragment Frag

           TEXTURE2D(_ColorTex);
           SAMPLER(sampler_ColorTex);

           struct Attributes
           {
               float4 vertex : POSITION;
               float2 uv : TEXCOORD0;
           };
           
           struct Varyings
			{
				float4 pos : SV_POSITION;
				float2 uv  : TEXCOORD0;
			};

			Varyings Vert(Attributes IN)
			{
				Varyings OUT;
				OUT.pos = TransformObjectToHClip(IN.vertex.xyz);
				OUT.uv = IN.uv;
				return OUT;
			}
 
			half4 Frag(Varyings IN) : SV_Target
			{
				return half4(SAMPLE_TEXTURE2D(_ColorTex, sampler_ColorTex, IN.uv).xyz, 0.5);
			}
			ENDHLSL
       }
		Pass
       {
           Name "DrawEdgePass1"
           Tags{"LightMode" = "SRPDefaultUnlit"}
           ZWrite Off
           
           HLSLPROGRAM
           #pragma vertex Vert
           #pragma fragment Frag

           half4 _ColorEdge;
           half _Width;

           struct Attributes
           {
               float4 vertex : POSITION;
               float2 uv : TEXCOORD0;
               float3 normal : NORMAL; 
           };
           
           struct Varyings
			{
				float4 pos : SV_POSITION;
				float2 uv  : TEXCOORD0;
			};

			Varyings Vert(Attributes IN)
			{
				Varyings OUT;
				OUT.pos = TransformObjectToHClip(IN.vertex.xyz+IN.normal*_Width);
				OUT.uv = IN.uv;
				return OUT;
			}
 
			half4 Frag(Varyings IN) : SV_Target
			{
				return _ColorEdge;
			}
			ENDHLSL
       }
    }
    FallBack "Diffuse"
}
