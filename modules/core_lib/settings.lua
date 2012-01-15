Settings = {}

local function convertSettingValue(value)
  if type(value) == 'table' then
    if value.x and value.width then
      return recttostring(value)
    elseif value.x then
      return pointtostring(value)
    elseif value.width then
      return sizetostring(value)
    elseif value.r then
      return colortostring(value)
    else
      return value
    end
  elseif value == nil then
    return ''
  else
    return tostring(value)
  end
end

function Settings.exists(key)
  return g_configs.exists(key)
end

function Settings.remove(key)
  g_configs.remove(key)
end

function Settings.set(key, value)
  g_configs.set(key, convertSettingValue(value))
end

function Settings.setList(key, list)
  g_configs.setList(key, list)
end

function Settings.setDefault(key, value)
  if Settings.exists(key) then return false end
  Settings.set(key, value)
  return true
end

function Settings.get(key, default)
  if not Settings.exists(key) and default ~= nil then
    Settings.set(key, default)
  end
  return g_configs.get(key)
end

function Settings.getList(key)
  return g_configs.getList(key)
end

function Settings.getString(key, default)
  return Settings.get(key, default)
end

function Settings.getInteger(key, default)
  return tonumber(Settings.get(key, default))
end

function Settings.getNumber(key, default)
  return tonumber(Settings.get(key, default))
end

function Settings.getBoolean(key, default)
  return toboolean(Settings.get(key, default))
end

function Settings.getPoint(key, default)
  return topoint(Settings.get(key, default))
end

function Settings.getRect(key, default)
  return torect(Settings.get(key, default))
end

function Settings.getSize(key, default)
  return tosize(Settings.get(key, default))
end

function Settings.getColor(key, default)
  return tocolor(Settings.get(key, default))
end

function Settings.getColor(key, default)
  return tocolor(Settings.get(key, default))
end
