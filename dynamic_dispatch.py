# created by <@!856229099952144464> (greyblue92#6547)
# improved by <@!675937585624776717> (Crowthebird#1649)

import sys
import traceback

from functools import wraps
from itertools import islice, takewhile
from inspect import (_VAR_KEYWORD, _VAR_POSITIONAL,
                     signature as get_signature, Parameter)
from typing import (Any, Callable, Iterable,
                   Iterator, Mapping, Tuple)

AnyCallable = Callable[[Any], Any]
ParamList = list[Parameter]

cache: dict[str,
            list[tuple[AnyCallable,
                       tuple[type, ...]]]] = {}

DEFAULT_EXC: Exception = Exception("match not found")
HAS_EXC_GROUPS: bool = sys.version_info > (3, 11, 0, 'alpha', 3)

def dynamic_dispatch(function: AnyCallable) -> AnyCallable:
    function_key: str = f"{function.__module__}.{function.__qualname__}"
    signatures: list[tuple[AnyCallable,
                           ParamList]] = cache.setdefault(function_key, [])
    signature: ParamList = list(get_signature(function)
                                .parameters
                                .values())
    signatures.append((function, signature))
    
    @wraps(function)
    def wrapper(*args: tuple[Any, ...],
                **kwargs: dict[str, Any]) -> AnyCallable:
        best_match: AnyCallable
        best_matches: Iterator[AnyCallable] = find_best_matches(args, kwargs,
                                                                signatures)
        err: BaseException | list[BaseException] = DEFAULT_EXC
        result: Any = None
        
        for best_match, signature in best_matches:
            try:
                result = best_match(*args, **kwargs)
            except BaseException as _temp:
                if HAS_EXC_GROUPS:
                    _temp.add_note(f"{signature}")
                else:
                    _temp = (signature, _temp)
                if isinstance(err, list):
                    err.append(_temp)
                elif err is not DEFAULT_EXC:
                    err = [err, _temp]
                else:
                    err = _temp
            else:
                return result
        if isinstance(err, list):
            if HAS_EXC_GROUPS:
                raise ExceptionGroup("all matches returned an error", err)
            else:
                print("all matches returned an error")
                for signature, exc in err[:-1]:
                    print(f"ERROR: {signature}")
                    traceback.print_exception(type(exc), exc, exc.__traceback__)
                signature, exc = err[-1]
                print(f"ERROR: {signature}")
                raise exc
        else:
            raise err
    
    return wrapper

def find_best_matches(positional_arguments: tuple[Any, ...],
                    keyword_arguments: dict[str, Any],
                    signatures: list[tuple[AnyCallable,
                                           ParamList]],
        ) -> Iterator[tuple[AnyCallable, ParamList]]:
    key: Any
    value: Any
    object: Any
    key_type: type
    value_type: type
    index: int
    signature_index: int
    obj_index: int
    POSARGS_LENGTH: int = len(positional_arguments)
    ARGS_LENGTH: int = POSARGS_LENGTH + len(keyword_arguments)
    TWO_CONST: int = 2
    scores: list[int] = [0] * len(signatures)
    function: AnyCallable
    signature: ParamList
    arguments: list[Any] = (list(positional_arguments)
                            + list(keyword_arguments.values()))
    keyword_names: list[str] = list(keyword_arguments.keys())
    zipped: tuple[AnyCallable, tuple[type, ...]]
    
    for signature_index, (function, signature) in enumerate(signatures):
        index: int = 0
        adjusted_index: int = 0
        while index < ARGS_LENGTH:
            if len(signature) <= adjusted_index:
                scores[signature_index] = 0
                break
            parameter: Parameter = signature[adjusted_index]
            annotation: type = parameter.annotation
            
            if parameter.kind is _VAR_POSITIONAL:
                for argument in islice(arguments, index, POSARGS_LENGTH):
                    argument_type = type(argument)
                    raw_type: type = annotation
                    if hasattr(annotation, "__origin__"):
                        raw_type = type(raw_type).__mro__[0]
                    
                    if argument_type is annotation:
                        scores[signature_index] += 25
                    elif (issubclass(argument_type, raw_type)
                          or issubclass(raw_type, argument_type)):
                        scores[signature_index] += 5
                    
                    if not hasattr(annotation, "__args__"):
                        continue
                    
                    type_parameters: type = annotation.__args__
                    if isinstance(argument, Tuple):
                        if not argument:
                            continue
                        
                        length_type_parameters: int = len(type_parameters)
                        length_argument: int = len(argument)
                        
                        if len(type_parameters) is TWO_CONST:
                            if type_parameters[-1] is ...:
                                type_parameter: type = type_parameters[0]
                                if type_parameter is Any:
                                    continue
                                
                                for object in argument:
                                    if issubclass(type(object),
                                                  type_parameter):
                                        scores[signature_index] += 3
                                    else:
                                        scores[signature_index] -= 1
                                
                                continue
                        if length_type_parameters < length_argument:
                            for (obj_index,
                                 type_parameter) in enumerate(type_parameters):
                                if issubclass(type(argument[obj_index]),
                                              type_parameter):
                                    scores[signature_index] += 3
                                else:
                                    scores[signature_index] -= 1
                            
                            scores[signature_index] -= 2
                            continue
                        
                        for obj_index, object in enumerate(argument):
                            if issubclass(type(object),
                                          type_parameters[obj_index]):
                                scores[signature_index] += 3
                            else:
                                scores[signature_index] -= 1
                        
                        if length_type_parameters == length_argument:
                            scores[signature_index] += 15
                    elif isinstance(argument, Mapping):
                        key_type, value_type = type_parameters
                        if key_type is Any:
                            if value_type is Any:
                                continue
                            
                            for value in argument.values():
                                if issubclass(type(value), value_type):
                                    scores[signature_index] += 3
                                else:
                                    scores[signature_index] -= 1
                        elif value_type is Any:
                            for key in argument.keys():
                                if issubclass(type(key), key_type):
                                    scores[signature_index] += 3
                                else:
                                    scores[signature_index] -= 1
                        else:
                            for key, value in argument.items():
                                if issubclass(type(key), key_type):
                                    scores[signature_index] += 5
                                else:
                                    scores[signature_index] -= TWO_CONST
                                
                                if issubclass(type(value), value_type):
                                    scores[signature_index] += 5
                                else:
                                    scores[signature_index] -= TWO_CONST
                    elif isinstance(argument, Iterable):
                        type_parameter: type = type_parameters[0]
                        if type_parameter is Any:
                            continue
                        
                        for object in argument:
                            if issubclass(type(object), type_parameter):
                                scores[signature_index] += 3
                            else:
                                scores[signature_index] -= 1
                
                index = POSARGS_LENGTH
                adjusted_index += 1
                continue
            elif parameter.kind is _VAR_KEYWORD:
                for argument in islice(arguments, index, ARGS_LENGTH):
                    argument_type = type(argument)
                    raw_type: type = annotation
                    if hasattr(annotation, "__origin__"):
                        raw_type = type(raw_type).__mro__[0]
                    
                    if argument_type is annotation:
                        scores[signature_index] += 25
                    elif (issubclass(argument_type, raw_type)
                          or issubclass(raw_type, argument_type)):
                        scores[signature_index] += 5
                    
                    if not hasattr(annotation, "__args__"):
                        continue
                    
                    type_parameters: type = annotation.__args__
                    if isinstance(argument, Tuple):
                        if not argument:
                            continue
                        
                        length_type_parameters: int = len(type_parameters)
                        length_argument: int = len(argument)
                        
                        if len(type_parameters) is TWO_CONST:
                            if type_parameters[-1] is ...:
                                type_parameter: type = type_parameters[0]
                                if type_parameter is Any:
                                    continue
                                
                                for object in argument:
                                    if issubclass(type(object),
                                                  type_parameter):
                                        scores[signature_index] += 3
                                    else:
                                        scores[signature_index] -= 1
                                
                                continue
                        if length_type_parameters < length_argument:
                            for (obj_index,
                                 type_parameter) in enumerate(type_parameters):
                                if issubclass(type(argument[obj_index]),
                                              type_parameter):
                                    scores[signature_index] += 3
                                else:
                                    scores[signature_index] -= 1
                            
                            scores[signature_index] -= 2
                            continue
                        
                        for obj_index, object in enumerate(argument):
                            if issubclass(type(object),
                                          type_parameters[obj_index]):
                                scores[signature_index] += 3
                            else:
                                scores[signature_index] -= 1
                        
                        if length_type_parameters == length_argument:
                            scores[signature_index] += 15
                    elif isinstance(argument, Mapping):
                        key_type, value_type = type_parameters
                        if key_type is Any:
                            if value_type is Any:
                                continue
                            
                            for value in argument.values():
                                if issubclass(type(value), value_type):
                                    scores[signature_index] += 3
                                else:
                                    scores[signature_index] -= 1
                        elif value_type is Any:
                            for key in argument.keys():
                                if issubclass(type(key), key_type):
                                    scores[signature_index] += 3
                                else:
                                    scores[signature_index] -= 1
                        else:
                            for key, value in argument.items():
                                if issubclass(type(key), key_type):
                                    scores[signature_index] += 5
                                else:
                                    scores[signature_index] -= TWO_CONST
                                
                                if issubclass(type(value), value_type):
                                    scores[signature_index] += 5
                                else:
                                    scores[signature_index] -= TWO_CONST
                    elif isinstance(argument, Iterable):
                        type_parameter: type = type_parameters[0]
                        if type_parameter is Any:
                            continue
                        
                        for object in argument:
                            if issubclass(type(object), type_parameter):
                                scores[signature_index] += 3
                            else:
                                scores[signature_index] -= 1
                
                break
            else:
                if index >= POSARGS_LENGTH:
                    if (keyword_names[index - POSARGS_LENGTH]
                            != parameter.name):
                        scores[signature_index] -= 20
                    else:
                        scores[signature_index] += 10
                
                argument: Any = arguments[index]
                argument_type: type = type(argument)
            
                raw_type: type = annotation
                if hasattr(annotation, "__origin__"):
                    raw_type = type(raw_type).__mro__[0]
                
                if argument_type is annotation:
                    scores[signature_index] += 25
                elif (issubclass(argument_type, raw_type)
                      or issubclass(raw_type, argument_type)):
                    scores[signature_index] += 5
                
                if not hasattr(annotation, "__args__"):
                    index += 1
                    adjusted_index += 1
                    continue
                
                type_parameters: type = annotation.__args__
                if isinstance(argument, Tuple):
                    if not argument:
                        index += 1
                        adjusted_index += 1
                        continue
                    
                    length_type_parameters: int = len(type_parameters)
                    length_argument: int = len(argument)
                    
                    if length_type_parameters is TWO_CONST:
                        if type_parameters[1] is ...:
                            type_parameter: type = type_parameters[0]
                            if type_parameter is Any:
                                index += 1
                                adjusted_index += 1
                                continue
                            
                            for object in argument:
                                if issubclass(type(object),
                                              type_parameter):
                                    scores[signature_index] += 3
                                else:
                                    scores[signature_index] -= 1
                            
                            index += 1
                            adjusted_index += 1
                            continue
                    if length_type_parameters < length_argument:
                        for (obj_index,
                             type_parameter) in enumerate(type_parameters):
                            if issubclass(type(argument[obj_index]),
                                          type_parameter):
                                scores[signature_index] += 3
                            else:
                                scores[signature_index] -= 1
                        
                        scores[signature_index] -= 2
                        index += 1
                        adjusted_index += 1
                        continue
                    
                    for obj_index, object in enumerate(argument):
                        if issubclass(type(object),
                                      type_parameters[obj_index]):
                            scores[signature_index] += 3
                        else:
                            scores[signature_index] -= 1
                    if length_type_parameters == length_argument:
                        scores[signature_index] += 15
                elif isinstance(argument, Mapping):
                    key_type, value_type = type_parameters
                    if key_type is Any:
                        if value_type is Any:
                            continue
                        
                        for value in argument.values():
                            if issubclass(type(value), value_type):
                                scores[signature_index] += 3
                            else:
                                scores[signature_index] -= 1
                    elif value_type is Any:
                        for key in argument.keys():
                            if issubclass(type(key), key_type):
                                scores[signature_index] += 3
                            else:
                                scores[signature_index] -= 1
                    else:
                        for key, value in argument.items():
                            if issubclass(type(key), key_type):
                                scores[signature_index] += 5
                            else:
                                scores[signature_index] -= TWO_CONST
                            
                            if issubclass(type(value), value_type):
                                scores[signature_index] += 5
                            else:
                                scores[signature_index] -= TWO_CONST
                elif isinstance(argument, Iterable):
                    type_parameter: type = type_parameters[0]
                    if type_parameter is Any:
                        continue
                    
                    for object in argument:
                        if issubclass(type(object), type_parameter):
                            scores[signature_index] += 3
                        else:
                            scores[signature_index] -= 1
                
                index += 1
                adjusted_index += 1
    
    scored: list[tuple[int,
                       tuple[AnyCallable, ParamList]]] = sorted(zip(scores, signatures),
                                                                key=lambda i: i[0])
    max_score: int = scored[-1][0]
    return ((tie[1][0], tie[1][1])
            for tie in takewhile(lambda x: x[0] == max_score,
                                 reversed(scored)))

if __name__ == "__main__":
    @dynamic_dispatch
    def foo(x: int, y: float, **kwargs):
        return x+y+sum(map(ord, kwargs.values()))
    
    @dynamic_dispatch
    def foo(x: int, y: float, *, z: list[int]):
        return x+y+sum(z)
    
    @dynamic_dispatch
    def foo(x: int, y: float):
        return x+y
    
    print("foo(2, 5.5, z='\x01', a='\x05')={}\nfoo(2, 5.5, z=[1,2,3])={}\nfoo(2, 5.5)={}".format(foo(2, 5.5, z='\x01', a='\x05'), foo(2, 5.5, z=[1,2,3]), foo(2, 5.5)))
    
    @dynamic_dispatch
    def bar(x: Iterable[int]):
        return ''.join(map(str,x))
    
    @dynamic_dispatch
    def bar(y: Iterable[str | int]):
        return ','.join(y)
    
    print(f"{bar([5,6])=}\n{bar(x=[5,6])=}")
    print(f"{bar(['5','6'])=}")
    
    @dynamic_dispatch
    def baz(x: Tuple[int, ...]):
        return sum(x)
    
    @dynamic_dispatch
    def baz(x: Tuple[int, int]):
        return x[0] - x[1]
    
    print(f"{baz((5,7,1))=}\n{baz((1,))=}\n{baz((2, 3))=}")
    baz(5)
