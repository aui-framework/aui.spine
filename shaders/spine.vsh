uniform {
  mat4 transform
}

input {
  [0] vec2 pos
  [1] vec2 uv
  [2] vec4 lightColor
  [3] vec4 darkColor
}

inter {
  vec2 uv
  vec4 lightColor
  vec4 darkColor
}

entry {
    sl_position = uniform.transform * vec4(input.pos.xy, 0, 1)
    inter.uv = input.uv
    inter.lightColor = input.lightColor
    inter.darkColor = input.darkColor
}
