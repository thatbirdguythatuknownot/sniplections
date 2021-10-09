from types import CodeType

def safe_eval_norecurse(input_string, allowed_names=None, builtins_dict=None, compiled_name="<string>"):
    if allowed_names is None:
        allowed_names = {}
    if builtins_dict is None:
        builtins_dict = {}
    stack = [compile(input_string, compiled_name, "eval")]
    stack_push = stack.append
    stack_pop = stack.pop
    indexlist = [0]
    indexlist_append = indexlist.append
    indexlist_pop = indexlist.pop
    breakloop = False
    while indexlist:
        code = stack[-1]
        for obj in code.co_consts[indexlist[-1]:]:
            indexlist[-1] += 1
            if isinstance(obj, CodeType):
                indexlist_append(0)
                stack_push(obj)
                breakloop = True
                break
        if breakloop:
            breakloop = False
            continue
        else:
            indexlist_pop()
        for name in code.co_names:
            if name not in allowed_names:
                raise NameError(f"Use of {name} not allowed")
        stack_pop()
    return eval(code, {"builtins": builtins_dict}, allowed_names)
