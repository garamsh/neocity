-- Native edits for #262; run once on the unmodified 053e0df kit sources.
local sprite = app.activeSprite
assert(sprite and sprite.colorMode == ColorMode.RGB, "Open an RGB kit source")
local kind = app.params.kind
local cel = sprite.cels[1]
local image = cel.image
local pc = app.pixelColor

if kind == "throat" or kind == "acid" or kind == "aquarium" then
  for pixel in image:pixels() do
    local value = pixel()
    local r, g, b, a = pc.rgbaR(value), pc.rgbaG(value), pc.rgbaB(value), pc.rgbaA(value)
    local amount = 0
    if kind == "throat" and r > g * 1.45 and b > g * 1.2 then
      amount = math.min(0.57, math.max(r, b) / 255 * 0.57)
    elseif kind == "acid" and g > r * 1.15 and g > b * 1.15 then
      amount = 0.46
    elseif kind == "aquarium" then
      if g > r * 1.15 and b > r * 1.1 then amount = 0.43 end
      if r > b * 1.35 and g > b * 1.1 then amount = 0.48 end
    end
    if amount > 0 and a > 0 then
      pixel(pc.rgba(math.floor(r * (1 - amount) + 19 * amount),
                    math.floor(g * (1 - amount) + 22 * amount),
                    math.floor(b * (1 - amount) + 39 * amount), a))
    end
  end
elseif kind == "deck" or kind == "aquarium_deck" then
  local function dot(x, y, rgb)
    if x >= 0 and x < image.width and y >= 0 and y < image.height then
      if pc.rgbaA(image:getPixel(x, y)) > 0 then
        image:drawPixel(x, y, pc.rgba(rgb[1], rgb[2], rgb[3], 255))
      end
    end
  end
  local function box(x, y, w, h, rgb)
    for yy = y, y + h - 1 do
      for xx = x, x + w - 1 do dot(xx, yy, rgb) end
    end
  end
  local bands = kind == "deck" and 6 or 1
  for band = 0, bands - 1 do
    local base = band * 126
    -- The 18-pixel contact rim and all existing alpha stay untouched.
    local top = kind == "deck" and 18 or 45
    local bottom = kind == "deck" and 58 or 76
    for y = top, bottom do
      for x = 2, image.width - 3 do
        local phase = x % 144
        local rust = math.floor(math.abs(math.sin(math.floor(x / 3) * 12.9898 +
                      math.floor(y / 2) * 78.233 + band * 7)) * 43758.5453) % 61
        local old = image:getPixel(x, base + y)
        local rgb = {math.min(92, pc.rgbaR(old) + 24),
                     math.min(70, pc.rgbaG(old) + 17),
                     math.min(88, pc.rgbaB(old) + 20)}
        if y < top + 3 then rgb = {103, 76, 79}
        elseif y > bottom - 4 then rgb = {22, 20, 35}
        elseif rust < 5 then rgb = {102, 56, 46}
        elseif rust < 10 then rgb = {74, 46, 47}
        end
        if phase < 3 then rgb = {17, 17, 29} end
        if phase == 3 then rgb = {115, 77, 68} end
        dot(x, base + y, rgb)
      end
    end
    for x = 132, image.width - 18, 288 do
      box(x, base + top + 2, 11, bottom - top + 13, {23, 21, 34})
      box(x + 2, base + top + 2, 7, bottom - top + 10, {79, 56, 59})
      box(x + 2, base + top + 3, 2, bottom - top + 7, {129, 87, 69})
      box(x + 4, base + top + 7, 3, 3, {175, 134, 94})
      box(x + 4, base + bottom + 1, 3, 3, {175, 134, 94})
    end
    for x = 12, image.width - 12, 144 do
      box(x, base + top + 7, 3, 3, {151, 111, 80})
      box(x + 1, base + top + 8, 2, 2, {39, 29, 39})
      box(x + 116, base + bottom - 11, 3, 3, {151, 111, 80})
      box(x + 117, base + bottom - 10, 2, 2, {39, 29, 39})
    end
    for x = 64 + band * 17, image.width - 28, 576 do
      box(x - 5, base + top + 6, 31, 15, {27, 22, 34})
      box(x - 3, base + top + 8, 27, 10, {102, 57, 39})
      box(x, base + top + 10, 20, 5, {207, 117, 42})
      box(x + 4, base + top + 10, 10, 2, {248, 184, 81})
      box(x + 7, base + top + 8, 2, 10, {41, 29, 36})
      box(x + 16, base + top + 8, 2, 10, {41, 29, 36})
    end
    if kind == "deck" and (band == 1 or band == 4) then
      for _, x in ipairs({32, 2750}) do
        -- Small service pipe/cable groups hang below the walking envelope.
        box(x, base + 65, 65, 9, {19, 18, 30})
        box(x + 2, base + 66, 61, 5, {77, 48, 45})
        box(x + 3, base + 66, 59, 1, {131, 85, 58})
        box(x + 3, base + 70, 59, 2, {40, 34, 46})
        box(x + 17, base + 67, 8, 2, {102, 59, 45})
        box(x + 35, base + 68, 6, 2, {105, 58, 44})
        for _, offset in ipairs({6, 50}) do
          box(x + offset, base + 63, 5, 12, {31, 27, 39})
          box(x + offset + 1, base + 63, 3, 10, {83, 70, 78})
          box(x + offset + 1, base + 64, 1, 8, {131, 101, 89})
          box(x + offset + 2, base + 64, 1, 1, {183, 143, 106})
        end
        box(x + 60, base + 64, 6, 11, {37, 30, 41})
        box(x + 61, base + 65, 4, 8, {98, 62, 49})
        box(x + 61, base + 65, 3, 1, {163, 111, 71})
        for i = 0, 35 do
          local yy = base + 77 + math.floor(13 * math.sin(i / 35 * math.pi))
          box(x + 15 + i, yy, 2, 3, {29, 22, 35})
          dot(x + 15 + i, yy, {107, 55, 55})
        end
      end
    end
  end
  if kind == "aquarium_deck" then
    for y = 0, image.height - 1 do
      image:drawPixel(image.width - 1, y, image:getPixel(0, y))
    end
  end
else
  error("Unknown polish kind")
end
sprite:saveAs(sprite.filename)
