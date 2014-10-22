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

-- Generator for a JSON definition of the parsed file

require "util"

-- 
function annotationsOfType(annotations, type)
	local r = {}
	for k, v in ipairs(annotations) do
		if v.type == type then
			table.insert (r, v)
		end
	end
	
	return r
end

function annotationsToJson(annotations)
	local r = {}
	for k, v in pairs(annotationsOfType (annotations, "custom")) do
		table.insert (r, { name = v.name, value = v.value })
	end
	
	return r
end

function functionsToJson(methods, type)
	local r = {}
	for k, v in pairs (findAll (methods, 'type', type)) do
		table.insert (r, {
			name = v.name,
			annotations = annotationsToJson (v.annotations),
			resultType = v.returnType,
			argumentNames = elementList (v.arguments, 'name'),
			argumentTypes = elementList (v.arguments, 'type')
		})
	end
	return r
end

function enumsToJson(enums)
	local r = {}
	for k, v in pairs (enums) do
		r[v.name] = {
			annotations = annotationsToJson (v.annotations),
			values = onEach (v.elements, function (c) return tonumber(c) end)
		}
	end
	return r
end

function isFieldWritable(variable)
	return (variable.setter == '' and variable.getter == '') or
	       (variable.getter ~= '' and variable.setter ~= '')
end

function fieldsToJson(fields)
	local r = {}
	for k, v in pairs (fields) do
		r[v.name] = {
			annotations = annotationsToJson (v.annotations),
			type = v.type,
			readOnly = not isFieldWritable (v)
		}
	end
	return r
end

function classToJson(class)
	return {
		annotations = annotationsToJson(class.annotations),
		bases = keys (class.bases),
		memberMethods = functionsToJson (class.methods, 'member'),
		staticMethods = functionsToJson (class.methods, 'static'),
		constructors = functionsToJson (class.methods, 'constructor'),
		enums = enumsToJson (class.enums),
		fields = fieldsToJson (class.variables)
	}
end

function fileToJson(file)
	local r = {}
	for k, v in pairs (findAll (definitions.classes, 'file', file)) do
		r[v.name] = classToJson (v)
	end
	
	return r
end

-- 
out = { }
for k, v in ipairs (tria.sourceFiles) do
	out[v] = fileToJson (v)
end

write (json.serialize (out))
