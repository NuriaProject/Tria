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

-- Generator for MetaObject classes for use with the NuriaProject Framework.

-------------------------------------------------------------- Utility functions
require "util"

function qByteArray(data)
	if data == '' then
		return 'QByteArray ()'
	else
		return 'QByteArrayLiteral("' .. data:escaped() .. '")'
	end
end

function tableToSwitch(t, key, addBreak, offset)
	local offset = offset or 0
	local switch = "switch (" .. key .. ") {\n"
	local body = ""
	local prevKey, prevCode = "", ""
	
	-- Find duplicate cases and eliminate those
	for k, v in pairs(t) do
		if v == prevCode then
			t[prevKey] = -1
		else
			prevCode = v
		end
		prevKey = k
	end
	
	-- Generate switch body
	for k, v in pairs(t) do
		if type(k) == 'number' then k = k - offset end
		local cur = "case " .. k .. ":"
		
		if v == -1 then
			cur = cur .. " // Fallthrough\n"
		elseif v:find ("\n") then
			cur = cur .. "\n" .. indentCode (2, v) .. "\n"
		else
			cur = cur .. " " .. v .. "\n"
		end
		
		if addBreak then cur = cur .. "break;\n" end
		
		body = body .. cur
	end
	
	-- Body is empty
	if body == '' then
		return "(void) " .. key .. ";\n", true
	end
	
	-- Done.
	return switch .. body .. "}\n", false
end

function tableToEnum(name, t)
	local enum = "enum " .. name .. " {\n"
	
	for k, v in pairs(t) do
		enum = enum .. "  " .. v .. " = " .. k .. ",\n"
	end
	
	return enum .. "};\n"
end

---------------------------------------------------------------------- Functions
function sourceFileListComment()
	if #tria.sourceFiles == 1 then return tria.sourceFiles[1] .. "\n" end
	
	local r = "\n"
	for k, v in ipairs (tria.sourceFiles) do
		r = r .. " *   " .. v .. "\n"
	end
	
	return r
end

function writeHeader()
	write ("/*******************************************************************************\n" ..
	       " * Meta-code generated by Tria [" .. tria.compileTime .. " " .. tria.compileDate .. "]\n" ..
	       " * Source file(s): " .. sourceFileListComment () ..
	       " * Date: " .. tria.currentDateTime .. "\n" ..
	       " * LLVM version: " .. tria.llvmVersion .. "\n" ..
	       " *\n" ..
	       " * W A R N I N G!\n" ..
	       " * This code is auto-generated. All changes you make WILL BE LOST!\n" ..
	       "*******************************************************************************/\n\n" ..
	       "/* For access to private QMetaType methods. */\n" ..
	       "#define Q_NO_TEMPLATE_FRIENDS\n" ..
	       "#include <nuria/metaobject.hpp>\n" ..
	       "#include <nuria/variant.hpp>\n" ..
	       "#include <QByteArray>\n" ..
	       "#include <QMetaType>\n" ..
	       "#include <QVector>\n")
end

function writeInclude(file)
	write ("#include \"" .. file .. "\"\n")
end

function writeClassDeclareMetatype(class)
	if class.isFakeClass then return end
	
	writeDeclareMetatype (class.name .. '*', true)
	if class.hasValueSemantics then
		writeDeclareMetatype (class.name, true)
	end
end

function shouldDeclareMetatype(name)
	if not name then return false end
	if table.containsValue (definitions.declaredTypes, name) or
	   table.containsValue (definitions.avoidedTypes, name) then
		return false
	end
	return true
end

function writeDeclareMetatype(name, fullyDeclared)
	fullyDeclared = (fullyDeclared == nil) and true or fullyDeclared
	
	local typedef = definitions.typedefs[name]
	local macro = fullyDeclared and 'NURIA_DECLARE_METATYPE' or 'Q_DECLARE_OPAQUE_POINTER'
	
	if not fullyDeclared and name:find (',') then
		name = '(' .. name .. ')'
	end
	
	if not shouldDeclareMetatype (name) or
	   (typedef and not shouldDeclareMetatype (typedef)) then
		return
	end
	
	table.insert (definitions.declaredTypes, name)
	write (macro .. "(" .. name .. ")\n")
end

function escapeName(name)
	return name:gsub ("[^A-Za-z0-9]", "_")
end

function metaObjectClassName(name)
	return "tria_" .. escapeName (name) .. "_metaObject"
end

function classMetaTypeId(class, asPointer)
	if class.isFakeClass then return "0" end
	if asPointer then return "qMetaTypeId< " .. class.name .. " * > ()" end
	
	if class.hasValueSemantics and
	   not table.containsValue (definitions.avoidedTypes, class.name) then
		return "qMetaTypeId< " .. class.name .. " > ()"
	else
		return "0"
	end
end

function metaTypeId(class)
	if type(class) ~= 'string' then class = class.name end
	return "qMetaTypeId< " .. class .. " > ()"
end

function converterName(conv)
	return "nuria_convert_" .. escapeName (conv.fromType) .. "_to_" .. escapeName (conv.toType)
end

function writeMemberConverter(conv)
	if table.containsValue (definitions.avoidedTypes, conv.fromType) or
	   table.containsValue (definitions.avoidedTypes, conv.toType) then
		return
	end
	
	-- 
	local isStatic = (conv.type == 'static')
	local isCtor = (conv.type == 'constructor')
	local call = isStatic and (conv.toType .. '::') or 'from->'
	local copy = not isCtor and not conv.isConst and ('  ' .. conv.fromType .. ' copy (*from);\n') or ''
	local var = (isCtor or conv.isConst) and '*from' or 'copy'
	
	if not isStatic and not isCtor and not conv.isConst then call = 'copy.' end
	
	call = call .. conv.methodName .. ' ('
	if isCtor then call = conv.toType .. ' (' end
	if isCtor or isStatic then call = call .. var end
	call = call .. ');'
	
	-- function methodInvoke(class, method, args, type)
	local name = converterName(conv)
	write ("inline static bool " .. name .. "(const QtPrivate::AbstractConverterFunction *, " ..
	       "const void *in, void *out) {\n" ..
	       "  const " .. conv.fromType .. " *from = " .. reinterpretCast ('const ' .. conv.fromType, 'in') .. ";\n" ..
	       "  " .. conv.toType .. " *to = " .. reinterpretCast (conv.toType, 'out') .. ";\n" ..
	       copy ..
	       "  *to = " .. call .. "\n" ..
	       "  return true;\n" ..
	       "}\n\n")
end

function writeMemberConverters(class)
	for k, v in pairs(class.conversions) do
		writeMemberConverter(v)
	end
	
end

function annotationsOfType(annotations, type)
	local r = {}
	for k, v in ipairs(annotations) do
		if v.type == type then
			table.insert (r, v)
		end
	end
	
	return r
end

function annotationCountSwitch(t)
	local r = {}
	for k, v in spairs(t) do
		local anno = annotationsOfType (v.annotations, "custom")
		table.insert (r, "return " .. #anno .. ";")
	end
	
	return indentCode (6, tableToSwitch (r, "index", false, 1)) .. " break;\n"
end

function writeAnnotationCountFunc(class)
	local anno = annotationsOfType (class.annotations, "custom")
	write ("  int _annotationCount (int category, int index) const {\n" ..
	       "    (void)index;\n" ..
	       "    switch (category) {\n" ..
	       "    case ObjectCategory: return " .. #anno .. ";\n" ..
	       "    case MethodCategory:\n" .. annotationCountSwitch (class.methods) ..
	       "    case FieldCategory:\n" .. annotationCountSwitch (class.variables) ..
	       "    case EnumCategory:\n" .. annotationCountSwitch (class.enums) ..
	       "    }\n\n" ..
	       "    return 0;\n" ..
	       "  }\n\n")
end

function annotationKeyValueSwitch(annotations, element, switchKey, func, indent)
	local r = {}
	local indent = indent or 6
	
	for i=1,#annotations do
		table.insert (r, "return " .. func(annotations[i][element], annotations[i]) .. ';')
	end
	
	local switch, isEmpty = tableToSwitch (r, switchKey, false, 1)
	return indentCode (indent, switch) .. " break;\n", isEmpty
end

function annotationNestedKeyValueSwitch(parent, element, func)
	local r = {}
	
	for k, v in spairs(parent) do
		local custom = annotationsOfType (v.annotations, "custom")
		local switch, empty = annotationKeyValueSwitch (custom, element, "nth", func, 0)
		table.insert (r, switch)
	end
	
	local switch, empty = tableToSwitch (r, "index", false, 1)
	if empty then return "", empty end
	
	return switch .. " break;\n", empty
end

function writeAnnotationFunc(class, type, name, field, func)
	local custom = annotationsOfType (class.annotations, "custom")
	local switchTable = {}
	
	local objCat, noObjCat = annotationKeyValueSwitch(custom, field, "nth", func, 0)
	local mCat, noMCat = annotationNestedKeyValueSwitch(class.methods, field, func)
	local fCat, noFCat = annotationNestedKeyValueSwitch(class.variables, field, func)
	local eCat, noECat = annotationNestedKeyValueSwitch(class.enums, field, func)
	
	if not noObjCat then switchTable["ObjectCategory"] = objCat end
	if not noMCat then switchTable["MethodCategory"] = mCat end
	if not noFCat then switchTable["FieldCategory"] = fCat end
	if not noECat then switchTable["EnumCategory"] = eCat end
	
	local funcSwitch, empty = tableToSwitch (switchTable, "category", false)
	if empty then
		funcSwitch = ''
	else
		funcSwitch = indentCode(4, funcSwitch) .. '\n'
	end
	
	write ("  " .. type .. " " .. name .. " (int category, int index, int nth) const {\n" ..
	       "    (void)category, (void)index, (void)nth;\n" ..
	       funcSwitch ..
	       "    return " .. type .. " ();\n" ..
	       "  }\n\n")
end

function annotationValue(value, annotation)
	if value == '' then
		return "QVariant ()"
	elseif annotation.typeName == 'QString' then
		return 'QStringLiteral("' .. value:escaped() .. '")'
	else
		return 'QVariant::fromValue (' .. value .. ')'
	end
end

function writeNameFunc(name, t, nameFunc)
	local r = {}
	
	for k, v in spairs (t) do
		table.insert (r, "return " .. qByteArray (nameFunc (v.name, v)) .. ";")
	end
	
	local head = "  QByteArray " .. name .. " (int index) {\n"
	local switch = indentCode (4, tableToSwitch (r, "index", false, 1)) .. "\n"
	local foot = "    return QByteArray ();\n  }\n\n"
	write(head .. switch .. foot)
end

function methodTypeToCppName(type)
	if type == 'constructor' then
		return 'Nuria::MetaMethod::Constructor'
	elseif type == 'member' then
		return 'Nuria::MetaMethod::Method'
	elseif type == 'static' then
		return 'Nuria::MetaMethod::Static'
	end
end

function writeMethodFunc(methods, prolog, default, func)
	local r = {}
	
	for i=1,#methods do
		r[i - 1] = 'return ' .. func(methods[i]) .. ';'
	end
	
	local head = "  " .. prolog .. " (int index) {\n"
	local switch = indentCode (4, tableToSwitch (r, "index", false)) .. "\n"
	local foot = "    return " .. default .. ";\n  }\n\n"
	write (head .. switch .. foot)
end

function methodArgumentNames(method)
	local elements = table.concat (onEach (elementList (method.arguments, 'name'), qByteArray), ", ")
	return "QVector< QByteArray > { " .. elements .. " }"
end

function methodArgumentTypes(method)
	local elements = table.concat (onEach (elementList (method.arguments, 'type'), qByteArray), ", ")
	return "QVector< QByteArray > { " .. elements .. " }"
end

function writeMethodInvokeFunc(class, name, func)
	local t = {}
	for i, v in ipairs(class.methods) do
		table.insert (t, i - 1, func (class, v))
	end
	
	local head = "  Nuria::Callback " .. name .. " (void *__instance, int index) {\n" ..
	             "    (void)__instance;\n"
	local switch = indentCode (4, tableToSwitch (t, "index", false)) .. "\n"
	local foot = "    return Nuria::Callback ();\n  }\n\n"
	write (head .. switch .. foot)
end

function prototypeArguments(types)
	local r = {}
	for k, v in ipairs (types) do
		local type = v.type
		if not v.isPodType and not (v.isReference and not v.isConst) then
			type = 'const ' .. type .. '&'
		end
		table.insert (r, type .. ' ' .. v.name)
	end
	
	return table.concat (r, ', ')
end

function methodArguments(method)
	return prototypeArguments (method.arguments)
end

function reinterpretCast(class, variable)
	local variable = variable or '__instance'
	if type (class) ~= 'string' then class = class.name end
	return 'reinterpret_cast< ' .. class .. ' * > (' .. variable .. ')'
end

function methodInvoke(class, method, args, typeName)
	local args = args or table.concat (elementList (method.arguments, 'name'), ', ')
	local typeName = typeName or method.type
	local method = (type(method) == 'string') and method or method.name
	local class = (type(class) == 'string') and class or class.name
	
	if class.isFakeClass then
		return method .. '(' .. args .. ')'
	elseif typeName == 'constructor' then
		if args ~= '' then args = ' (' .. args .. ')' end
		return 'new ' .. class .. args
	elseif typeName == 'member' then
		return reinterpretCast(class) .. '->' .. method .. '(' .. args .. ')'
	elseif typeName == 'static' then
		return class .. '::' .. method .. '(' .. args .. ')'
	end
	
end

function qualifiedType(type)
	local pre = type.isConst and 'const ' or ''
	local suf = type.isReference and '&' or ''
	return pre .. type.type .. suf
end

function functionPointerType(class, method)
	local prefix = method.type == 'member' and class.name .. '::' or ''
	local suffix = method.isConst and 'const' or ''
	local args = { }
	
	for k, v in ipairs (method.arguments) do
		table.insert (args, qualifiedType (v))
	end
	
	local result = qualifiedType (method.returnType)
	return result .. '(' .. prefix .. '*)(' .. table.concat (args, ',') .. ')' .. suffix
	
end

function constructorCallback(class, method)
	local args = methodArguments (method)
	return 'Nuria::Callback::fromLambda ([](' .. args .. ') { return ' .. methodInvoke (class, method) .. '; });'
end

function isTypeNonConstRef(type)
	return type.isReference and not type.isConst
end

function useMethodTrampoline(class, method)
	for i=1,#method.arguments do
		local cur = method.arguments[i]
		if isTypeNonConstRef(cur) then
			return true
		end
	end
	
	return isTypeNonConstRef(method.returnType) or method.hasOptionalArguments
end

function lambdaCallback(class, method)
	local call, toType = '', ''
	local args = methodArguments (method)
	local passArgs = table.concat (elementList (method.arguments, 'name'), ', ')
	
	-- 
	if method.type == 'member' then
		call = reinterpretCast (class) .. '->'
	elseif not class.isFakeClass then
		call = class.name .. '::'
	end
	
	-- 
	if method.returnType.type ~= 'void' then call = 'return ' .. call end
	if isTypeNonConstRef(method.returnType) then
		toType = '-> ' .. method.returnType.type .. ' '
	end
	
	-- 
	call = call .. method.name .. ' (' .. passArgs .. ')'
	return 'Nuria::Callback::fromLambda ([__instance](' .. args .. ') ' .. toType .. '{ ' .. call .. '; });'
end

function methodToCallback(class, method)
	
	-- More complex Callbacks for specific methods
	if method.type == 'constructor' then
		return constructorCallback (class, method)
	elseif useMethodTrampoline (class, method) then
		return lambdaCallback (class, method)
	end
	
	-- Direct callbacks
	local fullName = class.isFakeClass and method.name or class.name .. '::' .. method.name
	local cast = functionPointerType(class, method)
	local cbArgs = '(' .. cast .. ')&' .. fullName
	if method.type == 'member' then
		cbArgs = reinterpretCast (class) .. ', ' .. cbArgs
	end
	
	return 'Nuria::Callback (' .. cbArgs .. ');'
	
end

function requireAnnotation(method)
	for k, v in pairs (method.annotations) do
		if v.type == 'require' then
			return v.value
		end
	end
	
	return nil
end

function nuriaChecker(inheritFrom, args, check, isStatic)
	local pre = isStatic and 'static ' or ''
	return 'struct NuriaChecker : public ' .. inheritFrom .. ' { ' ..
	       pre .. 'bool __nuria_check (' .. args ..
	       ') { return (' .. check .. '); } };'
end

function invokeChecker(class, method, check, success, failure)
	local isStatic = (method.type ~= 'member')
	local checkerCall = ''
	local passArgs = table.concat (elementList (method.arguments, 'name'), ', ')
	local struct = nuriaChecker (class.name, methodArguments (method), check, isStatic)
	
	if failure ~= '' then
		failure = ' else { ' .. failure .. ' }'
	end
	
	if isStatic then
		checkerCall = 'NuriaChecker::__nuria_check'
	else
		checkerCall = 'reinterpret_cast< NuriaChecker * > (__instance)->__nuria_check'
	end
	
	return struct .. ' if (' .. checkerCall .. '(' .. passArgs .. ')) { ' ..
	       success .. ' }' .. failure
end

function defaultConstruct(type, isPod, isCtor)
	if type == 'void' then return '' end
	if isPod then return '0;' end
	if isCtor then return '(' .. type.type .. ' *)nullptr;' end
	
	return type.type .. ' ();'
end

function shouldSkipMethod(class, method)
	return class.hasPureVirtuals and method.type == 'constructor'
end

function methodUnsafeCallback(class, method)
	if shouldSkipMethod(class, method) then
		return 'return Nuria::Callback (); // Not constructable'
	end
	return 'return ' .. methodToCallback(class, method)
end

function methodCallback(class, method)
	local check = requireAnnotation (method)
	
	-- Shortcut for methods without NURIA_REQUIRE annotation
	if not check or shouldSkipMethod(class, method) then
		return 'return _methodUnsafeCallback (__instance, index);'
	end
	
	-- 
	local fail = defaultConstruct (method.returnType, method.returnTypeIsPod, method.type == 'constructor')
	local call = methodInvoke(class, method)
	if method.returnType.type ~= 'void' then
		call = 'return QVariant::fromValue (' .. call .. ');'
		fail = 'return QVariant ();'
	else
		call = call .. ';'
	end
	
	-- 
	local inner = invokeChecker (class, method, check, call, fail)
	return 'return Nuria::Callback::fromLambda ([__instance](' ..
	        methodArguments (method) .. ') { ' .. inner .. ' });'
end

function methodArgumentTest(class, method)
	local check = requireAnnotation (method)
	if not check or shouldSkipMethod(class, method) then
		return 'return Nuria::Callback (&returnTrue);'
	end
	
	-- 
	local inner = invokeChecker (class, method, check, 'return true;', 'return false;')
	return 'return Nuria::Callback::fromLambda ([__instance](' ..
	        methodArguments (method) .. ') { ' .. inner .. ' });'
end

function writeFieldFunc(class, proto, voids, default, func)
	local t = {}
	
	for k, v in ipairs (class.variables) do
		table.insert (t, func(v.name, v, class))
	end
	
	write ('  ' .. proto .. ' {\n    ' .. voids .. '\n' ..
	       indentCode (4, tableToSwitch (t, 'index', false, 1)) ..
	       '\n    return ' .. default .. ';\n  }\n\n')
end

function isFieldWritable(variable)
	return (variable.setter == '' and variable.getter == '') or
	       (variable.getter ~= '' and variable.setter ~= '') or
	       (variable.getter == '' and variable.setter ~= '')
end

function isFieldReadable(variable)
	return (variable.setter == '' and variable.getter == '') or
	       (variable.getter ~= '' and variable.setter ~= '') or
	       (variable.getter ~= '' and variable.setter == '')
end

function fieldAccess(name, data)
	if isFieldWritable (data) then
		if isFieldReadable (data) then
			return "return Nuria::MetaField::ReadWrite;"
		end
		
		return 'return Nuria::MetaField::WriteOnly;'
	end
	
	return "return Nuria::MetaField::ReadOnly;"
end

function fieldRead(name, data, class)
	local f = name
	
	-- Write-only fields
	if not isFieldReadable (data) then return 'return QVariant ();' end
	if data.getter ~= '' then f = data.getter .. ' ()' end
	return 'return QVariant::fromValue (' .. reinterpretCast (class) .. '->' .. f .. ');'
end

function fieldWrite(name, data, class)
	if not isFieldWritable (data) then
		return 'return false;'
	end
	
	-- 
	local argName = (data.setterArgName ~= '') and data.setterArgName or data.name
	local args = prototypeArguments ({ { type = data.type, isPodType = data.isPodType, name = argName } })
	local check = requireAnnotation (data)
	local checker = check and nuriaChecker (class.name, args, check, isStatic) or ''
	
	-- 
	local converter = '  if (__value.userType () != ' .. metaTypeId(data.type) .. ') {\n' ..
	                  '    QVariant v (__value);\n' ..
	                  '    return (v.convert (' .. metaTypeId(data.type) .. ')) ? _fieldWrite (index, __instance, v) : false;\n' ..
	                  '  }\n'
	
	-- 
	local cast = reinterpretCast (class) .. '->'
	local value = '__value.value< ' .. data.type .. ' > ()'
	if data.type == 'QVariant' then value = '__value' end
	
	local setter = cast .. data.name .. ' = ' .. value .. '; return true'
	if data.setter ~= '' then
		setter = cast .. data.setter .. ' (' .. value .. ')'
		if data.setterReturnsBool then
			setter = 'return ' .. setter
		else
			setter = setter .. '; return true'
		end
	end
	
	-- Shortcut for fields not using a checker
	if not check then
		return converter .. setter .. ';'
	end
	
	-- 
	local call = reinterpretCast ('NuriaChecker') .. '->__nuria_check (' .. value .. ')'
	return '{\n' .. converter .. '\n  ' .. checker .. '\n  if (!' .. call .. ') { return false; } ' .. setter .. ';\n}'
	
end

function writeEnumFunc(class, proto, voids, default, func)
	local t = {}
	for k, v in spairs (class.enums) do
		table.insert (t, func(v, class))
	end
	
	write ('  ' .. proto .. ' {\n    ' .. voids .. '\n' ..
	       indentCode (4, tableToSwitch (t, 'index', false, 1)) ..
	       '\n    return ' .. default .. ';\n  }\n\n')
end

function enumElementKey(enum)
	local t = onEach (keys (enum.elements), function(e)
		return 'return ' .. qByteArray (e) .. ';'
	end)
	return tableToSwitch (t, 'at', false, 1) .. 'break;'
end

function enumElementValue(enum, class)
	local t = onEach (keys (enum.elements), function(e)
		return 'return ' .. class.name .. '::' .. e .. ';'
	end)
	return tableToSwitch (t, 'at', false, 1) .. 'break;'
end

function shouldFilterMethod(m)
	local Avoid = function(type)
		return definitions.declareTypes[type.type] == false or
		       table.containsValue (definitions.avoidedTypes, type.type)
	end
	
	if m.type == 'constructor' then return false end
	if Avoid (m.returnType) then return true end
	for i=1,#m.arguments do
		if Avoid (m.arguments[1]) then return true end
	end
	
	return false
end

function filterClassMethods(class)
	local Keep = function(k, m) return not shouldFilterMethod (m) end
	return filtered (class.methods, Keep)
end

function writeGateCall()
		write ("  void gateCall (GateMethod method, int category, int index, int nth, \n" ..
	       "                 void *result, void *additional) override {\n" ..
	       "    switch (method) {\n" ..
	       "    case Nuria::MetaObject::GateMethod::ClassName:\n" ..
	       "      RESULT(QByteArray) = _className (); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::MetaTypeId:\n" ..
	       "      RESULT(int) = _metaTypeId (); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::PointerMetaTypeId:\n" ..
	       "      RESULT(int) = _pointerMetaTypeId (); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::BaseClasses:\n" ..
	       "      RESULT(QVector< QByteArray >) = _baseClasses (); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::AnnotationCount:\n" ..
	       "      RESULT(int) = _annotationCount (category, index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::MethodCount:\n" ..
	       "      RESULT(int) = _methodCount (); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::FieldCount:\n" ..
	       "      RESULT(int) = _fieldCount (); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::EnumCount:\n" ..
	       "      RESULT(int) = _enumCount (); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::AnnotationName:\n" ..
	       "      RESULT(QByteArray) = _annotationName (category,  index, nth); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::AnnotationValue:\n" ..
	       "      RESULT(QVariant) = _annotationValue (category,  index, nth); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::MethodName:\n" ..
	       "      RESULT(QByteArray) = _methodName (index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::MethodType:\n" ..
	       "      RESULT(Nuria::MetaMethod::Type) = _methodType (index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::MethodReturnType:\n" ..
	       "      RESULT(QByteArray) = _methodReturnType (index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::MethodArgumentNames:\n" ..
	       "      RESULT(QVector< QByteArray >) = _methodArgumentNames (index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::MethodArgumentTypes:\n" ..
	       "      RESULT(QVector< QByteArray >) = _methodArgumentTypes (index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::MethodCallback:\n" ..
	       "      RESULT(Nuria::Callback) = _methodCallback (additional, index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::MethodUnsafeCallback:\n" ..
	       "      RESULT(Nuria::Callback) = _methodUnsafeCallback (additional, index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::MethodArgumentTest:\n" ..
	       "      RESULT(Nuria::Callback) = _methodArgumentTest (additional, index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::FieldName:\n" ..
	       "      RESULT(QByteArray) = _fieldName (index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::FieldType:\n" ..
	       "      RESULT(QByteArray) = _fieldType (index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::FieldRead:\n" ..
	       "      RESULT(QVariant) = _fieldRead (index, additional); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::FieldWrite: {\n" ..
	       "      void **argData = reinterpret_cast< void ** > (additional);\n" ..
	       "      const QVariant &value = *reinterpret_cast< QVariant * > (argData[1]);\n" ..
	       "      RESULT(bool) = _fieldWrite (index, argData[0], value);\n" ..
	       "    } break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::FieldAccess:\n" ..
	       "      RESULT(Nuria::MetaField::Access) = _fieldAccess (index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::EnumName:\n" ..
	       "      RESULT(QByteArray) = _enumName (index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::EnumElementCount:\n" ..
	       "      RESULT(int) = _enumElementCount (index); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::EnumElementKey:\n" ..
	       "      RESULT(QByteArray) = _enumElementKey (index, nth); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::EnumElementValue:\n" ..
	       "      RESULT(int) = _enumElementValue (index, nth); break;\n" ..
	       "    case Nuria::MetaObject::GateMethod::DestroyInstance:\n" ..
	       "      _destroy (additional); break;\n" ..
	       "    }\n" ..
	       "  }\n")
end

function writeClassDef(name, class)
	local Key = function(k) return k end
	local Name = function(k, v) return v.name end
	local MethodType = function(v) return methodTypeToCppName(v.type) end
	local MethodReturnType = function(v) return qByteArray(v.returnType.type) end
	local FieldType = function(k, v) return 'return ' .. qByteArray(v.type) .. ';' end
	local EnumElemCount = function(v) return 'return ' .. table.length (v.elements) .. ';' end
	
	if class.hasValueSemantics then writeMemberConverters(class) end
	class.methods = filterClassMethods (class)
	
	-- Write short methods
	write ("class Q_DECL_HIDDEN " .. metaObjectClassName (name) .. " : public Nuria::MetaObject {\n" ..
	       "public:\n" ..
	       "  QByteArray _className () const {\n" ..
	       "    return " .. qByteArray (name) .. ";\n" ..
	       "  }\n\n" ..
	       "  int _metaTypeId () const {\n" ..
	       "    return " .. classMetaTypeId (class, false) .. ";\n" ..
	       "  }\n\n" ..
	       "  int _pointerMetaTypeId () const {\n" ..
	       "    return " .. classMetaTypeId (class, true) .. ";\n" ..
	       "  }\n\n" ..
	       "  void _destroy (void *__instance) {\n" ..
	       "    delete " .. reinterpretCast (class) .. ";\n" ..
	       "  }\n\n" ..
	       "  QVector< QByteArray > _baseClasses () {\n" ..
	       "    return QVector< QByteArray > { " .. table.concat (onEach (keys (class.bases), qByteArray), ", ") .. " };\n" ..
	       "  }\n\n" ..
	       "  int _methodCount () const {\n" ..
	       "    return " .. table.length(class.methods) .. ";\n" ..
	       "  }\n\n" ..
	       "  int _fieldCount () const {\n" ..
	       "    return " .. table.length(class.variables) .. ";\n" ..
	       "  }\n\n" ..
	       "  int _enumCount () const {\n" ..
	       "    return " .. table.length(class.enums) .. ";\n" ..
	       "  }\n\n"
	       )
	
	-- Write more complex functions
	writeAnnotationCountFunc (class)
	writeAnnotationFunc (class, "QByteArray", "_annotationName", "name", qByteArray)
	writeAnnotationFunc (class, "QVariant", "_annotationValue", "value", annotationValue)
	writeNameFunc ("_methodName", class.methods, Name)
	writeNameFunc ("_fieldName", class.variables, Name)
	writeNameFunc ("_enumName", class.enums, Name)
	writeMethodFunc (class.methods, 'Nuria::MetaMethod::Type _methodType',
	                 methodTypeToCppName ('member'), MethodType)
	writeMethodFunc (class.methods, 'QByteArray _methodReturnType',
	                 'QByteArray ()', MethodReturnType)
	writeMethodFunc (class.methods, 'QVector< QByteArray > _methodArgumentNames',
	                 'QVector< QByteArray > ()', methodArgumentNames)
	writeMethodFunc (class.methods, 'QVector< QByteArray > _methodArgumentTypes',
	                 'QVector< QByteArray > ()', methodArgumentTypes)
	writeMethodInvokeFunc (class, '_methodUnsafeCallback', methodUnsafeCallback)
	writeMethodInvokeFunc (class, '_methodCallback', methodCallback)
	writeMethodInvokeFunc (class, '_methodArgumentTest', methodArgumentTest)
	writeFieldFunc (class, 'Nuria::MetaField::Access _fieldAccess (int index)', '',
	                'Nuria::MetaField::NoAccess', fieldAccess)
	writeFieldFunc (class, 'QByteArray _fieldType (int index)', '', 'QByteArray ()', FieldType)
	writeFieldFunc (class, 'QVariant _fieldRead (int index, void *__instance)',
	                '(void)__instance;', 'QVariant ()', fieldRead)
	writeFieldFunc (class, 'bool _fieldWrite (int index, void *__instance, const QVariant &__value)',
	                '(void)__instance; (void)__value;', 'false', fieldWrite)
	writeEnumFunc (class, 'int _enumElementCount (int index)', '', '0', EnumElemCount)
	writeEnumFunc (class, 'QByteArray _enumElementKey (int index, int at)',
	               '(void)at;', 'QByteArray ()', enumElementKey)
	writeEnumFunc (class, 'int _enumElementValue (int index, int at)',
	               '(void)at;', '-1', enumElementValue)
	writeGateCall ()
	
	-- End
	write ("};\n\n")
end

function registerMetaType(typeName)
	return "qRegisterMetaType< " .. typeName .. " > ();\n"
end

function writeRegisterMetatypes()
	for k, v in pairs (definitions.declareTypes) do
		if v then write ("    " .. registerMetaType (k)) end
	end
end

function writeRegisterMetaObjectClass(class)
	local m = metaObjectClassName(class.name)
	write ("    Nuria::MetaObject::registerMetaObject (new " .. m .. ");\n")
	
	if not class.isFakeClass then
		write ("    " .. registerMetaType (class.name .. ' *'))
	end
	
	if class.hasValueSemantics then
		write ("    " .. registerMetaType (class.name))
	end
	
	write ("\n")
end

function writeRegisterConverter(conv)
	if table.containsValue (definitions.avoidedTypes, conv.fromType) or
	   table.containsValue (definitions.avoidedTypes, conv.toType) then
		return
	end
	
	if conv.type == 'constructor' then
		write ("    QMetaType::registerConverter< " .. conv.fromType .. ", " .. conv.toType .. " > ();\n")
		return
	end
	
	local var = escapeName (conv.fromType) .. "_to_" .. escapeName (conv.toType)
	write ("    static QtPrivate::AbstractConverterFunction " .. var ..
	       "(&" .. converterName (conv) .. ");\n" ..
	       "    QMetaType::registerConverterFunction (&" .. var ..
	       ", " .. metaTypeId (conv.fromType) .. ", " .. metaTypeId (conv.toType) .. ");\n")
	
end

function writeRegisterConverters(class)
	if not class.hasValueSemantics then return end
	for k, v in pairs(class.conversions) do writeRegisterConverter(v) end
end

function writeInstantiatorClass()
	local prefix = escapeName(tria.outFile)
	local class = prefix .. "_Register"
	
	write ("struct Q_DECL_HIDDEN " .. class .. " {\n" ..
	       "  " .. class .. " () {\n")
	
	writeRegisterMetatypes()
	write ("\n")
	
	for k, v in pairs(definitions.classes) do
		writeRegisterMetaObjectClass(v)
		writeRegisterConverters(v)
	end
	
	write ("  }\n};\n\n" ..
	       prefix .. "_Register " .. prefix .. "_instantiator;\n")
end

--------------------------------------------------------------------------------

writeHeader ()
for k, v in ipairs(tria.sourceFiles) do writeInclude (v) end
write ("\n")

-- Q_DECLARE_METATYPEs
for k,v in pairs(definitions.classes) do writeClassDeclareMetatype (v) end
for k,v in pairs(definitions.declareTypes) do writeDeclareMetatype (k, v) end

-- 
write ("\n" ..
       "#define RESULT(Type) *reinterpret_cast< Type * > (result)\n" ..
       "namespace TriaObjectData {\n\n")

write (tableToEnum ("Categories", {
       [0] = "ObjectCategory",
       [1] = "MethodCategory",
       [2] = "FieldCategory",
       [3] = "EnumCategory"
}))

write ("\n" ..
       "static bool returnTrue () { return true; }\n\n")

-- Class definitions
for k,v in pairs (definitions.classes) do writeClassDef (k, v) end
writeInstantiatorClass ();

-- Close namespace
write ("}\n\n" ..
       "#undef RESULT\n")
