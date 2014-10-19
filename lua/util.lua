--  Copyright (c) 2014, The Nuria Project
--  This software is provided 'as-is', without any express or implied
--  warranty. In no event will the authors be held liable for any damages
--  arising from the use of this software.
--  Permission is granted to anyone to use this software for any purpose,
--  including commercial applications, and to alter it and redistribute it
--  freely, subject to the following restrictions:
--    1. The origin of this software must not be misrepresented; you must not
--       claim that you wrote the original software. If you use this software
--       in a product, an acknowledgment in the product documentation would be
--       appreciated but is not required.
--    2. Altered source versions must be plainly marked as such, and must not be
--       misrepresented as being the original software.
--    3. This notice may not be removed or altered from any source
--       distribution.

-- Utility functions useful for writing generators.

-- Iterates over 't' and returns the element count
function table.length(t)
	local count = 0
	
	for k in pairs(t) do count = count + 1 end
	return count
end

-- Returns 'true' if 't' contains 'value'
function table.containsValue(t, value)
	for k, v in pairs(t) do
		if v == value then return true end
	end
	return false
end

-- Searches 't' for 'value' and if found, returns its key (or nil)
function table.keyOfValue(t, value)
	for k, v in pairs(t) do
		if v == value then return k end
	end
	return nil
end

-- Splits the string by any character in 'sep'
function string:split(sep)
        local fields = {}
	
        self:gsub("([^" .. sep .. "]+)", function(c)
		table.insert (fields, c)
	end)
	
        return fields
end

-- Escapes the string
function string:escaped()
	return self:gsub ("\"", "\\\"")
end

-- Calls func(value, key) on each element in 'table'
function onEach(table, func)
	for k,v in pairs(table) do
		table[k] = func(v, k)
	end
	
	return table
end

-- Returns an array of the 'name' field in all values in 't'
function elementList(t, name)
	local r = {}
	for k, v in ipairs (t) do
		table.insert (r, v[name])
	end
	
	return r
end

-- Returns an array of all keys in 't'
function keys(t)
	local result = {}
	for k in pairs(t) do
		table.insert (result, k)
	end
	
	return result
end

-- Returns an array of all values in 't'
function values(t)
	local result = {}
	for k, v in pairs(t) do
		table.insert (result, v)
	end
	
	return result
end

-- Finds all values in 't' where 'key' is 'value'
function findAll(t, key, value)
	local r = {}
	for k, v in pairs (t) do
		if v[key] == value then table.insert (r, k, v) end
	end
	return r
end

-- Iterates over 't' by sorting by key.
function spairs(t)
	local k = keys (t)
	table.sort (k)
	local f = function(t, i)
		i = i + 1
		local v = t[k[i]]
		if v ~= nil then
			return i, v
		end
		return nil
	end
	return f, t, 0
end

-- Indents the string 'code' by 'level' spaces
function indentCode(level, code)
	if level < 1 then
		return code
	end
	
	local pre = string.rep (" ", level)
	return table.concat (onEach(code:split ("\n"), function(line)
		return pre .. line;
	end), "\n")
end
