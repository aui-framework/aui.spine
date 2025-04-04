uniform {
  2D albedo
}

inter {
  vec2 uv
  vec4 lightColor
  vec4 darkColor
}

output {
  [0] vec4 albedo
}

entry {
    vec4 texColor = uniform.albedo[inter.uv]
    float alpha = texColor.a * inter.lightColor.a
    output.albedo = vec4(((texColor.a - 1) * inter.darkColor.a + 1 - texColor.rgb) * inter.darkColor.rgb + texColor.rgb * inter.lightColor.rgb, alpha)
}
