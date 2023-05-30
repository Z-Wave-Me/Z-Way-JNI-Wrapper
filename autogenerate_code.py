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
    
    def __repr__(self):
        return "{%s, %s, %s, %s}" % (self.name, self.type, self.default, self.description)

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

# Return type of C++ object based on C type
def GetParamType(type):
        if type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                return 'ByteArray'
        if type == 'const ZWNODE *' or type == 'ZWNODE *':
                return 'IntArray'
        if type == 'ZWCSTR':
                return 'NativeString'
        return type

# Return mangling string for parameter value transmited to C call
def GetParamValueMangling(type):
        if type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                return '%s.ptr()'
        if type == 'const ZWNODE *' or type == 'ZWNODE *':
                return '%s.ptr()'
        if type == 'ZWCSTR':
                return '%s.ptr()'
        return '%s'

# Return default value of C++ variable
def GetParamDefaultValue(type, value):
        if type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                return 'ByteArray()'
        if type == 'const ZWNODE *' or type == 'ZWNODE *':
                return 'IntArray()'
        if type == 'ZWCSTR':
                return 'NativeString()'
        return value

# Return parser for Java parameter to transform it to C++ object
def GetParamParser(type, name, i):
        if type == 'ZWBOOL':
                return '(ZWBOOL) info[%i]->BooleanValue()' % i
        if type == 'ZJobCustomCallback' and name == 'successCallback':
                return 'zwayContext->GetSuccessCallback(callbackId, info[%i])' % i
        if type == 'ZJobCustomCallback' and name == 'failureCallback':
                return 'zwayContext->GetFailureCallback(callbackId, info[%i])' % i
        if type == 'unsigned short' or type == 'unsigned short int' or type == 'unsigned int' or type == 'int' or type == 'speed_t' or type == 'ZWBYTE' or type == 'ZWNODE':
                return '(%s) info[%i]->IntegerValue()' % (type, i)
        if type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                return 'ByteArray(info[%i])' % i
        if type == 'const ZWNODE *' or type == 'ZWNODE *':
                return 'IntArray(info[%i])' % i
        if type == 'ZWCSTR':
                return 'NativeString(info[%i])' % i
        if type == 'float':
                return '(float) info[%i]->NumberValue()' % i
        if type == 'time_t':
                return '(time_t) info[%i]->Uint32Value()' % i
        if type == 'size_t':
                return '(_t) info[%i]->Uint32Value()' % i

        raise ValueError("")

def GetParamJavaDeclaration(param):
        type = param.type
        if type == 'ZWBOOL':
                jtype = 'jboolean'
        elif type == 'unsigned short' or type == 'unsigned short int' or type == 'unsigned int' or type == 'int' or type == 'speed_t' or type == 'ZWBYTE' or type == 'ZWNODE':
                jtype = 'jint'
        elif type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                jtype = '???'
        elif type == 'const ZWNODE *' or type == 'ZWNODE *':
                jtype = '???'
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

def GetParamSignature(param):
        type = param.type
        if type == 'ZWBOOL':
                return 'Z'
        if type == 'unsigned short' or type == 'unsigned short int' or type == 'unsigned int' or type == 'int' or type == 'speed_t' or type == 'ZWBYTE' or type == 'ZWNODE':
                return 'I'
        if type == 'const ZWBYTE *' or type == 'ZWBYTE *':
                return '?'
        if type == 'const ZWNODE *' or type == 'ZWNODE *':
                return '?'
        if type == 'ZWCSTR':
                return 'Ljava/lang/String;'
        if type == 'float':
                return 'F'
        if type == 'time_t':
                return 'J'
        if type == 'size_t':
                return 'J'
        
        raise ValueError("Error parsing parameter " + str(param))

# Translate C parameter names into Java
def CamelCase(name):
        return name.split('_')[0] + ''.join(map(lambda w: w.capitalize(), name.split('_')[1:]))

##################### FC ###################


##################### CC ###################

def ParseCC():
        ccTemplates = {}
        paramsDescriptions = Params()
        funcDescription = []
        dhDescriptions = []
        funcReport = ""
        lastParamName = ""
        className = ""
        classID = ""
        
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
                        paramsDescriptions.pop(-1) # remove successCallback, failureCallback, callbackArg too
                        paramsDescriptions.pop(-1)
                        paramsDescriptions.pop(-1)
                        paramsC = paramsC[2:-3]

                        for p in paramsC:
                            try:
                                paramsDescriptions.Get(p[1]).type = p[0]
                            except:
                                raise ValueError("%s: Can not match parameter %s in [%s]" % (funcCName, p[1], ', '.join(map(lambda x: x.name, paramsDescriptions))))

                        cmd = Command(funcCName)
                        cmd.params = paramsDescriptions
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
                        if templating:
                                template.append(lines[i])
                        m = re.search(".*BEGIN AUTOGENERATED CC TEMPLATE.*", line)
                        if m is not None:
                                templating = True
                                template = []
                                _m = m
                
                i = i + 1

        with open(file, "w") as f:
                f.write("".join(lines))

def GenerateCodeCC(template):
        code = []
        for cc in commandClasses:
                for cmd in cc.commands:
                        for line in template:
                                code.append(line
                                        .replace("%function_name%", cmd.name)
                                        .replace("%function_short_name%", cmd.name.replace("zway_cc_", ""))
                                        .replace("%function_short_camel_case_name%", CamelCase(cmd.name.replace("zway_cc_", "")))
                                        .replace("%params%", "".join(map(lambda p: p.name + ", ", cmd.params)))
                                        .replace("%params_java_declarations%", "".join(map(GetParamJavaDeclaration, cmd.params)))
                                        .replace("%params_signature%", "".join(map(GetParamSignature, cmd.params)))
                                )
        return code

clean = True if len(sys.argv) >= 2 and sys.argv[1] == "clean" else False

commandClasses = ParseCC()

ParseFile("jni_zway_wrapper.c", clean)
ParseFile("ZWay.java", clean)
