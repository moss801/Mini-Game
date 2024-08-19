Shader "Unlit/DissolveShader"
{
    Properties
    {
        _ColorTex ("ColorTexture", 2D) = "white" {}
        _NoiseTex ("NoiseTexture", 2D) = "white" {}
        _DissolveAmount ("Dissolve Amount", Range(0,1)) = 0
        _LineWidth ("Dissolve Line Width", Range(0,0.2)) = 0.1
        _DissolveOuterColor ("Dissolve Outer Color", Color) = (1,0,0,1)
        _DissolveInnerColor ("Dissolve Outer Color", Color) = (1,0,0,1)
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
            Name "DissolvePass"
            Tags{"LightMode" = "UniversalForward"}
            
            Cull Off
            
            HLSLPROGRAM
            #pragma vertex vert
            #pragma fragment frag

            sampler2D _ColorTex;

            sampler2D _NoiseTex;

            half _DissolveAmount;
            half _LineWidth;
            half4 _DissolveOuterColor;
            half4 _DissolveInnerColor;
            float4 _ColorTex_ST;
            float4 _NoiseTex_ST;

            struct Attributes
            {
                float4 vertex : POSITION;
                float3 normal : NORMAL;
                float2 uvColor : TEXCOORD0;
            };

            struct Varyings
            {
                float4 pos : SV_POSITION;
                float3 normal : NORMAL;
                float3 worldPos : TEXCOORD2;
                float2 uvColor : TEXCOORD0;
                float2 uvNoise : TEXCOORD1;
            };

            Varyings vert (Attributes v)
            {
                Varyings o;
                o.pos = TransformObjectToHClip(v.vertex);
                o.normal = TransformObjectToWorldNormal(v.normal);
                o.uvColor = TRANSFORM_TEX(v.uvColor, _ColorTex);
                o.uvNoise = TRANSFORM_TEX(v.uvColor, _NoiseTex);
                o.worldPos = mul(GetObjectToWorldMatrix(),v.vertex).xyz;
                return o;
            }

            half4 frag (Varyings i) : SV_Target
            {
                half noise = tex2D(_NoiseTex,i.uvNoise).r;
                half4 color = tex2D(_ColorTex,i.uvColor);
                float factor = noise - _DissolveAmount;
                clip(factor);
                half t = smoothstep(0,_LineWidth,factor);
                half4 DissolveColor = lerp(_DissolveOuterColor,_DissolveInnerColor,t);
                return lerp(color,DissolveColor,(1-t)*step(0.0001,_DissolveAmount));
                return factor;
            }
            ENDHLSL
        }
    }
}
