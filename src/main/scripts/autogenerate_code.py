#!/usr/bin/python2

import re
import os
import sys
import subprocess

libZWayDir = 'z-way-root/libzway/'

class CommandClass:
    def __init__(self, name, id):
        self.name = name
        self.id = id
        self.commands = []
    
    def __repr__(self):
        return "{Command Class %s, %i, %s}" % (self.name, self.id, self.commands)
        
class Command:
    def __init__(self, name):
        self.name = name
        self.params = None
    
    def __repr__(self):
        return "{Command %s, %s}" % (self.name, self.params)

class Param:
    def __init__(self, name):
        self.name = name
        self.default = None
        self.description = []
        self.type = None
        self.isLength = False
        self.dataName = None
    
    def __repr__(self):
        return "{%s, %s, %s, %s, %s}" % (self.name, self.type, self.default, self.description, "isLength" if self.isLength else "")

class Params(list):
    def Add(self, param):
        self.append(param)
    
    def Get(self, name):
        for p in self:
            if p.name == name:
                return p
        return None

# Returns True if parameter have pair with length. Length is always first to pointer parameter
def IsWithLength(params, i):
        if (params[i].type == "ZWBYTE" or params[i].type == "size_t" or params[i].type == "int" or params[i].type == "speed_t" or params[i].type == "short" or params[i].type == "unsigned int" or params[i].type == "unsigned short") and len(params) >= (i+1) and (params[i+1].type == "const ZWBYTE *" or params[i+1].type == "ZWBYTE *" or params[i+1].type == "const ZWNODE *" or params[i+1].type == "ZWNODE *"):
                return True
        return False

# Return mangling string for parameter value transmited to C call
def GetParamValueMangling(param):
        name = param.name
        type = param.type
        isLength = param.isLength
        dataName = param.dataName
        if type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                return 'c_byte_%(name)s' % {'name': name}
        if type == 'const ZWNODE *' or type == 'ZWNODE *':
                return 'c_int_%(name)s' % {'name': name}
        if type == 'ZWCSTR':
                return 'c_str_%(name)s' % {'name': name}
        if isLength:
                return 'c_length_%(name)s' % {'name': dataName}
        return name

# Return parser for Java parameter to transform it to C object
def GetParamParser(param):
        name = param.name
        type = param.type
        if type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                return 'jint *c_int_%(name)s = (*env)->GetIntArrayElements(env, %(name)s, NULL); jsize c_length_%(name)s = (*env)->GetArrayLength(env, %(name)s); ZWBYTE *c_byte_%(name)s = (ZWBYTE *)malloc(c_length_%(name)s); for (int i = 0; i < c_length_%(name)s; i++) c_byte_%(name)s[i] = (ZWBYTE)c_int_%(name)s[i];' % {'name': name}
        if type == 'const ZWNODE *' or type == 'ZWNODE *':
                return 'jint *c_int_%(name)s = (*env)->GetIntArrayElements(env, %(name)s, NULL); jsize c_length_%(name)s = (*env)->GetArrayLength(env, %(name)s);' % {'name': name}
        if type == 'ZWCSTR':
                return 'const char *c_str_%(name)s = (*env)->GetStringUTFChars(env, %(name)s, JNI_FALSE);' % {'name': name}
        return ''

# Return releas after parser for Java parameter to transform it to C object
def GetParamParserRelease(param):
        name = param.name
        type = param.type
        if type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                return 'free(c_byte_%(name)s); (*env)->ReleaseIntArrayElements(env, %(name)s, c_int_%(name)s, 0);' % {'name': name}
        if type == 'const ZWNODE *' or type == 'ZWNODE *':
                return '(*env)->ReleaseIntArrayElements(env, %(name)s, c_int_%(name)s, 0);' % {'name': name}
        if type == 'ZWCSTR':
                return ''
        return ''

def GetParamJNIDeclaration(param):
        type = param.type
        if type == 'ZWBOOL':
                jtype = 'jboolean'
        elif type == 'unsigned short' or type == 'unsigned short int' or type == 'unsigned int' or type == 'int' or type == 'speed_t' or type == 'ZWBYTE' or type == 'ZWNODE':
                jtype = 'jint'
        elif type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                jtype = 'jintArray'
        elif type == 'const ZWNODE *' or type == 'ZWNODE *':
                jtype = 'jintArray'
        elif type == 'ZWCSTR':
                jtype = 'jstring'
        elif type == 'float':
                jtype = 'jfloat'
        elif type == 'time_t':
                jtype = 'jlong'
        elif type == 'size_t':
                jtype = 'jlong'
        else:     
                raise ValueError("Error parsing parameter " + str(param))
        return jtype + ' ' + param.name + ', '

def GetParamJavaDeclaration(param):
        type = param.type
        if type == 'ZWBOOL':
                jtype = 'boolean'
        elif type == 'unsigned short' or type == 'unsigned short int' or type == 'unsigned int' or type == 'int' or type == 'speed_t' or type == 'ZWBYTE' or type == 'ZWNODE':
                jtype = 'int'
        elif type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                jtype = 'int[]'
        elif type == 'const ZWNODE *' or type == 'ZWNODE *':
                jtype = 'int[]'
        elif type == 'ZWCSTR':
                jtype = 'String'
        elif type == 'float':
                jtype = 'float'
        elif type == 'time_t':
                jtype = 'long'
        elif type == 'size_t':
                jtype = 'long'
        else:     
                raise ValueError("Error parsing parameter " + str(param))
        return jtype + ' ' + param.name + ', '

def GetParamSignature(param):
        type = param.type
        if type == 'ZWBOOL':
                return 'Z'
        if type == 'unsigned short' or type == 'unsigned short int' or type == 'unsigned int' or type == 'int' or type == 'speed_t' or type == 'ZWBYTE' or type == 'ZWNODE':
                return 'I'
        if type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                return '[I'
        if type == 'const ZWNODE *' or type == 'ZWNODE *':
                return '[I'
        if type == 'ZWCSTR':
                return 'Ljava/lang/String;'
        if type == 'float':
                return 'F'
        if type == 'time_t':
                return 'J'
        if type == 'size_t':
                return 'J'
        
        raise ValueError("Error parsing parameter " + str(param))

def IsParamNotLength(p):
        return not p.isLength

# Translate C parameter names into Java

def CamelCase(name):
        return name.split('_')[0][0].lower() + name.split('_')[0][1:] + ''.join(map(lambda w: w[0].upper() + w[1:], name.split('_')[1:]))

def CapitalizedCase(name):
        return ''.join(map(lambda w: w[0].upper() + w[1:], name.split('_')))

##################### FC ###################

def ParseFC():
        paramsDescriptions = Params()
        lastParamName = ""
        className = ""
        skippingIfDef = False
        
        functionClasses = []

        fcDefinition = open(libZWayDir + 'FunctionClassesPublic.h', 'r')

        for l in fcDefinition.readlines():
                if l.strip() == "":
                    paramsDescriptions = Params() # new function description started
                    lastParamName = ""
                    continue

                if l.startswith("#endif"):
                        skippingIfDef = False
                        continue
                if l.startswith("#ifdef"):
                        skippingIfDef = True
                        continue
                if skippingIfDef:
                        continue

                m = re.match("^// (@param:|@default:)? *(.*) *$", l)
                if m is not None:
                        mValue = m.groups()[1].strip(" ")
                        if m.groups()[0] == "@param:":
                                lastParamName = mValue
                                if mValue == "zway": # we skip zway parameter
                                    continue
                                paramsDescriptions.Add(Param(mValue))
                        elif m.groups()[0] == "@default:":
                                if paramsDescriptions.Get(lastParamName) is not None:
                                    paramsRegEx = " *(-?0x[0-9a-f]+|-?[0-9]+|NULL|TRUE|FALSE)"
                                    mm = re.match(paramsRegEx, mValue)
                                    if mm is None:
                                        raise ValueError("Default value definition incorrect. Current line is: %s" % l)
                                    paramsDescriptions.Get(lastParamName).default = mm.groups()[0]
                        continue

                if l.startswith("ZWEXPORT ZWError "):
                        prototypeC = l
                        paramsRegEx = " *, *([\w ]+[ \*]+) *(\w+)"
                        m = re.match("ZWEXPORT ZWError *(\w+)\(ZWay zway((%s)*)\)" % paramsRegEx, prototypeC)
                        if m is None:
                                raise ValueError("Error parsing line in search of C prototype string: %s" % prototypeC)
                        funcCName = m.groups()[0]
                        f = re.findall(paramsRegEx, m.groups()[1])
                        if f is None:
                                raise ValueError("Error parsing line in search of C prototype string: %s" % prototypeC)
                        paramsC = map(lambda x: (x[0].strip(' '), x[1]), f) # this match eats spaces too - strip them
                        if len(paramsDescriptions) != len(paramsC):
                            raise ValueError("Parameters description mismatch C declaration of %s\n%s != %s" % (prototypeC, paramsDescriptions, paramsC))
                        if map(lambda x: x.name, paramsDescriptions[-3:]) != ["successCallback", "failureCallback", "callbackArg"]:
                            raise ValueError("C successCallback, failureCallback, callbackArg arguments are missing")

                        paramsDescriptions.pop(-1) # remove successCallback
                        paramsDescriptions.pop(-1) # failureCallback and
                        paramsDescriptions.pop(-1) # callbackArg
                        paramsC = paramsC[0:-3]

                        for p in paramsC:
                            try:
                                paramsDescriptions.Get(p[1]).type = p[0]
                            except:
                                raise ValueError("%s: Can not match parameter %s in [%s]" % (funcCName, p[1], ', '.join(map(lambda x: x.name, paramsDescriptions))))

                        cmd = Command(funcCName)
                        cmd.params = paramsDescriptions
                        
                        for i in range(0, len(paramsDescriptions)):
                                if i != len(paramsDescriptions) - 1 and IsWithLength(paramsDescriptions, i):
                                        cmd.params[i].isLength = True
                                        cmd.params[i].dataName = cmd.params[i + 1].name
                        
                        functionClasses.append(cmd)

        fcDefinition.close()
        return functionClasses


##################### CC ###################

def ParseCC():
        paramsDescriptions = Params()
        lastParamName = ""
        className = ""
        
        commandClasses = []

        ccDefinition = open(libZWayDir + 'CommandClassesPublic.h', 'r')

        for l in ccDefinition.readlines():
                if l.strip() == "":
                    paramsDescriptions = Params() # new function description started
                    lastParamName = ""
                    continue

                m = re.match("^// Command Class (.*) \(0x([0-9a-fA-F]{2})/([0-9]+)\) //$", l)
                if m is not None:
                        className = m.groups()[0]
                        classIdNumFromHex = int(m.groups()[1], 16)
                        classIdNum = int(m.groups()[2], 10)
                        if classIdNumFromHex != classIdNum:
                                raise ValueError("Command Class Id does not match: %i != %i (%s)" % (classIdNumFromHex, classIdNum, className)) 
                        className = className.replace(" ", "").replace("-", "")
                        cc = CommandClass(className, classIdNum)
                        commandClasses.append(cc)
                        continue

                m = re.match("^// (@param:|@default:)? *(.*) *$", l)
                if m is not None:
                        mValue = m.groups()[1].strip(" ")
                        if m.groups()[0] == "@param:":
                                lastParamName = mValue
                                if mValue == "zway": # we skip zway parameter
                                    continue
                                paramsDescriptions.Add(Param(mValue))
                        elif m.groups()[0] == "@default:":
                                if paramsDescriptions.Get(lastParamName) is not None:
                                    paramsRegEx = " *(-?0x[0-9a-f]+|-?[0-9]+|NULL|TRUE|FALSE)"
                                    mm = re.match(paramsRegEx, mValue)
                                    if mm is None:
                                        raise ValueError("Default value definition incorrect. Current line is: %s" % l)
                                    paramsDescriptions.Get(lastParamName).default = mm.groups()[0]
                        continue

                if l.startswith("ZWEXPORT ZWError "):
                        prototypeC = l
                        paramsRegEx = " *, *([\w ]+[ \*]+) *(\w+)"
                        m = re.match("ZWEXPORT ZWError *(\w+)\(ZWay zway((%s)*)\)" % paramsRegEx, prototypeC)
                        if m is None:
                                raise ValueError("Error parsing line in search of C prototype string: %s" % prototypeC)
                        funcCName = m.groups()[0]
                        f = re.findall(paramsRegEx, m.groups()[1])
                        if f is None:
                                raise ValueError("Error parsing line in search of C prototype string: %s" % prototypeC)
                        paramsC = map(lambda x: (x[0].strip(' '), x[1]), f) # this match eats spaces too - strip them
                        if len(paramsDescriptions) != len(paramsC):
                            raise ValueError("Parameters description mismatch C declaration of %s\n%s != %s" % (prototypeC, paramsDescriptions, paramsC))
                        if map(lambda x: x.name, paramsDescriptions[:2]) != ["node_id", "instance_id"]:
                            raise ValueError("C node_id, instance_id arguments are missing")
                        if map(lambda x: x.name, paramsDescriptions[-3:]) != ["successCallback", "failureCallback", "callbackArg"]:
                            raise ValueError("C successCallback, failureCallback, callbackArg arguments are missing")

                        paramsDescriptions.pop(0)  # remove node_id and instance_id from params list
                        paramsDescriptions.pop(0)  # the list is in fact Param() object, so we can not reassign it
                        paramsDescriptions.pop(-1) # remove successCallback
                        paramsDescriptions.pop(-1) # failureCallback and
                        paramsDescriptions.pop(-1) # callbackArg
                        paramsC = paramsC[2:-3]

                        for p in paramsC:
                            try:
                                paramsDescriptions.Get(p[1]).type = p[0]
                            except:
                                raise ValueError("%s: Can not match parameter %s in [%s]" % (funcCName, p[1], ', '.join(map(lambda x: x.name, paramsDescriptions))))

                        cmd = Command(funcCName)
                        cmd.params = paramsDescriptions
                        
                        for i in range(0, len(paramsDescriptions)):
                                if i != len(paramsDescriptions) - 1 and IsWithLength(paramsDescriptions, i):
                                        cmd.params[i].isLength = True
                                        cmd.params[i].dataName = cmd.params[i + 1].name
                        
                        commandClasses[-1].commands.append(cmd)

        ccDefinition.close()
        return commandClasses

def ParseFile(file, clean):
        with open(file, "r") as f:
                lines = f.readlines()

        deleting = False
        templating = False
        
        L = len(lines)
        i = 0
        while i < L:
                line = lines[i]
                
                # delete the old autogenerated code
                if re.search("BEGIN AUTOGENERATED CC CODE", line) is not None:
                        deleting = True
                if re.search("END AUTOGENERATED CC CODE", line) is not None:
                        deleting = False
                        del lines[i]
                        L = L - 1
                        continue
                if re.search("BEGIN AUTOGENERATED FC CODE", line) is not None:
                        deleting = True
                if re.search("END AUTOGENERATED FC CODE", line) is not None:
                        deleting = False
                        del lines[i]
                        L = L - 1
                        continue
                if deleting:
                        del lines[i]
                        L = L - 1
                        continue
                
                if not clean:
                        # autogenerate code based on template
                        m = re.search("(.*)(END AUTOGENERATED CC TEMPLATE.*)", line)
                        if m is not None:
                                templating = False
                                templateLines = GenerateCodeCC(template)
                                templateLines.insert(0, _m.group(0).replace("TEMPLATE", "CODE") + " */\n")
                                templateLines.append(m.group(1)[:-2] + "/* " + m.group(2).replace("TEMPLATE", "CODE") + "\n")
                                tL = len(templateLines)
                                for j, insertLine in enumerate(templateLines):
                                        lines.insert(i + j + 1, insertLine)
                                L = L + tL
                                i = i + tL
                        m = re.search("(.*)(END AUTOGENERATED FC TEMPLATE.*)", line)
                        if m is not None:
                                templating = False
                                templateLines = GenerateCodeFCLine(template)
                                templateLines.insert(0, _m.group(0).replace("TEMPLATE", "CODE") + " */\n")
                                templateLines.append(m.group(1)[:-2] + "/* " + m.group(2).replace("TEMPLATE", "CODE") + "\n")
                                tL = len(templateLines)
                                for j, insertLine in enumerate(templateLines):
                                        lines.insert(i + j + 1, insertLine)
                                L = L + tL
                                i = i + tL
                        if templating:
                                template.append(lines[i])
                        m = re.search(".*BEGIN AUTOGENERATED CC TEMPLATE.*", line)
                        if m is not None:
                                templating = True
                                template = []
                                _m = m
                        m = re.search(".*BEGIN AUTOGENERATED FC TEMPLATE.*", line)
                        if m is not None:
                                templating = True
                                template = []
                                _m = m
                
                i = i + 1

        with open(file, "w") as f:
                f.write("".join(lines))

def GenerateCodeFCLine(template):
        code = []
        for fc in functionClasses:
                for line in template:
                        code.append(line
                                .replace("%function_name%", fc.name)
                                .replace("%function_short_name%", fc.name.replace("zway_fc_", ""))
                                .replace("%function_short_camel_case_name%", CamelCase(fc.name.replace("zway_fc_", "")))
                                .replace("%function_short_capitalized_name%", CapitalizedCase(fc.name.replace("zway_fc_", "")))
                                .replace("%function_command_camel_name%", CamelCase(CapitalizedCase(fc.name.replace("zway_fc_", "")).replace(CapitalizedCase(fc.name), "")))
                                .replace("%params_c%", "".join(map(lambda p: GetParamValueMangling(p) + ", ", fc.params)))
                                .replace("%params_java%", "".join(map(lambda p: p.name + ", ", filter(IsParamNotLength, fc.params))))
                                .replace("%params_jni_declarations%", "".join(map(GetParamJNIDeclaration, filter(IsParamNotLength, fc.params))))
                                .replace("%params_java_declarations%", "".join(map(GetParamJavaDeclaration, filter(IsParamNotLength, fc.params))))
                                .replace("%params_java_declarations_no_comma%", "".join(map(GetParamJavaDeclaration, filter(IsParamNotLength, fc.params)))[:-2])
                                .replace("%params_signature%", "".join(map(GetParamSignature, filter(IsParamNotLength, fc.params))))
                                .replace("%params_parser%", "".join(map(GetParamParser, filter(IsParamNotLength, fc.params))))
                                .replace("%params_parser_release%", "".join(map(GetParamParserRelease, filter(IsParamNotLength, fc.params))))
                        )
        
        return code

def GenerateCodeCCLine(line, cc, cmd = None):
        line = (line
                .replace("%cc_camel_case_name%", CamelCase(cc.name))
                .replace("%cc_capitalized_name%", CapitalizedCase(cc.name))
                .replace("%cc_id%", hex(cc.id))
        )
        
        if cmd is None:
                return line
        
        line = (line
                .replace("%function_name%", cmd.name)
                .replace("%function_short_name%", cmd.name.replace("zway_cc_", ""))
                .replace("%function_short_camel_case_name%", CamelCase(cmd.name.replace("zway_cc_", "")))
                .replace("%function_short_capitalized_name%", CapitalizedCase(cmd.name.replace("zway_cc_", "")))
                .replace("%function_command_camel_name%", CamelCase(CapitalizedCase(cmd.name.replace("zway_cc_", "")).replace(CapitalizedCase(cc.name), "")))
                .replace("%params_c%", "".join(map(lambda p: GetParamValueMangling(p) + ", ", cmd.params)))
                .replace("%params_java%", "".join(map(lambda p: p.name + ", ", filter(IsParamNotLength, cmd.params))))
                .replace("%params_jni_declarations%", "".join(map(GetParamJNIDeclaration, filter(IsParamNotLength, cmd.params))))
                .replace("%params_java_declarations%", "".join(map(GetParamJavaDeclaration, filter(IsParamNotLength, cmd.params))))
                .replace("%params_java_declarations_no_comma%", "".join(map(GetParamJavaDeclaration, filter(IsParamNotLength, cmd.params)))[:-2])
                .replace("%params_signature%", "".join(map(GetParamSignature, filter(IsParamNotLength, cmd.params))))
                .replace("%params_parser%", "".join(map(GetParamParser, filter(IsParamNotLength, cmd.params))))
                .replace("%params_parser_release%", "".join(map(GetParamParserRelease, filter(IsParamNotLength, cmd.params))))
        )
        
        return line


def GenerateCodeCC(template):
        code = []
        inFunc = False
        for cc in commandClasses:
                for line in template:
                        if re.search("(.*)(END AUTOGENERATED CMD TEMPLATE.*)", line) is not None:
                                inFunc = False
                                for cmd in cc.commands:
                                        for funcLine in funcLines:
                                                code.append(GenerateCodeCCLine(funcLine, cc, cmd))
                                continue
                        if re.search("(.*)(BEGIN AUTOGENERATED CMD TEMPLATE.*)", line) is not None:
                                funcLines = []
                                inFunc = True
                                continue
                        if inFunc:
                                funcLines.append(line)
                        else:
                                code.append(GenerateCodeCCLine(line, cc))
        return code

if len(sys.argv) < 3:
        print("Usage: " + sys.argv[0] + " (clean|generate) file1 [file2] ...]")
        exit(1)

clean = True if sys.argv[1] == "clean" else False

functionClasses = ParseFC()
commandClasses = ParseCC()

for i in range(2, len(sys.argv)):
        ParseFile(sys.argv[i], clean)
