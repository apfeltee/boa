
import re

def refactor_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    types_list = [
        'void', 'int', 'double', 'size_t', 'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t', 'int64_t', 'bool', 'char', 'va_list',
        'LitUInt', 'LitState', 'LitVm', 'LitFiber', 'LitValue', 'LitString', 'LitObject', 'LitModule', 'LitTable', 'LitChunk', 'LitValList',
        'LitByteList', 'LitUIntList', 'LitCallFrame', 'LitMap', 'LitClass', 'LitInstance', 'LitArray', 'LitUserdata', 'LitRange',
        'LitField', 'LitReference', 'LitScanner', 'LitParser', 'LitEmitter', 'LitCompiler', 'LitLocal', 'LitToken', 'LitExpression',
        'LitParameter', 'LitParamList', 'LitExprList', 'LitFileData', 'FILE', 'LitPrivList', 'LitLocList', 'LitParseRule',
        'NNStringBuffer', 'NNIOStream', 'LitResult', 'LitStatusCode', 'LitTokenType', 'LitPrecedence', 'LitExpressionType',
        'LitInstructionType', 'LitOpCode', 'NNPrMode', 'LitValType', 'LitFunctionType', 'LitErrorType', 'LitTableEntry',
        'LitBoundMethod', 'LitVarargArray', 'LitEvent', 'LitEventSystem', 'LitCompilerUpvalue', 'LitEmulatedFile',
        'LitArrayExpression', 'LitObjectExpression', 'LitSubscriptExpression', 'LitThisExpression', 'LitSuperExpression',
        'LitRangeExpression', 'LitTernaryExpression', 'LitInterpolationExpression', 'LitReferenceExpression',
        'LitExpressionStatement', 'LitBlockStatement', 'LitVarStatement', 'LitIfStatement', 'LitWhileStatement',
        'LitForStatement', 'LitContinueStatement', 'LitBreakStatement', 'LitFunctionStatement', 'LitReturnStatement',
        'LitMethodStatement', 'LitClassStatement', 'LitFieldStatement', 'clock_t'
    ]
    types_re = '(?:' + '|'.join(types_list) + r')\b\s*\*?'

    # Improved function finder that handles nested braces correctly
    def find_functions(text):
        # Matches function header and opening brace
        # Header: (type) (name)(args) followed by newline and {
        pattern = re.compile(r'^((?:[\w\*]+\s+)+)([\w]+)\s*\([^\)]*\)\s*\n\{', re.MULTILINE)
        pos = 0
        while True:
            match = pattern.search(text, pos)
            if not match:
                break
            
            start_func = match.start()
            header = match.group(0)
            func_name = match.group(2)
            
            brace_count = 1
            i = match.end()
            while brace_count > 0 and i < len(text):
                if text[i] == '{':
                    brace_count += 1
                elif text[i] == '}':
                    brace_count -= 1
                i += 1
            
            end_func = i
            body = text[match.end():end_func-1]
            yield start_func, end_func, header, func_name, body
            pos = end_func

    new_content = ""
    last_pos = 0
    
    for start, end, header, name, body in find_functions(content):
        new_content += content[last_pos:start]
        
        if name == 'lit_interpret_fiber':
            new_content += header + body + "}"
            last_pos = end
            continue
            
        all_decls = []
        
        # Match declarations: Type name [= init];
        # We match from start of line or after { or ;
        # This is more robust than just start of line
        decl_pattern = re.compile(r'([;\{\n]\s*)(' + types_re + r')\s+([^;=,\(\)]+?)(\s*=[^;]+)?\s*;')
        
        # For loops: for(Type name = init;
        for_pattern = re.compile(r'for\s*\(\s*(' + types_re + r')\s+([^;=]+?)\s*(=[^;]+)?\s*;')

        def sub_decl(m):
            prefix = m.group(1)
            t = m.group(2).strip()
            names = m.group(3).strip()
            init = m.group(4)
            # Avoid matching return statements or other keywords that might look like types
            if t in ['return', 'else', 'goto', 'break', 'continue']:
                return m.group(0)
            
            all_decls.append(f"    {t} {names};")
            if init:
                return f"{prefix}{names}{init};"
            return prefix.rstrip()

        def sub_for(m):
            t = m.group(1).strip()
            names = m.group(2).strip()
            init = m.group(3)
            all_decls.append(f"    {t} {names};")
            if init:
                return f"for({names}{init};"
            return f"for({names};"

        new_body = decl_pattern.sub(sub_decl, body)
        new_body = for_pattern.sub(sub_for, new_body)
        
        # Dedup decls
        seen = set()
        unique_decls = []
        for d in all_decls:
            if d not in seen:
                unique_decls.append(d)
                seen.add(d)
        
        decl_block = "\n".join(unique_decls)
        if decl_block:
            decl_block = "\n" + decl_block + "\n"
        
        # Clean up double newlines
        new_body = re.sub(r'\n\s*\n\s*\n', '\n\n', new_body)
        
        new_content += header + decl_block + new_body + "}"
        last_pos = end
        
    new_content += content[last_pos:]
    
    with open(filepath, 'w') as f:
        f.write(new_content)

if __name__ == "__main__":
    refactor_file('main.c')
