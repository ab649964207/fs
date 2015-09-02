"""
 This file contains all the necessary entities to define P3R domains and problems.
"""
from collections import OrderedDict
from util import is_action_parameter, is_int


class Variable(object):
    """
    A state variable, made up of a symbol and a number of arguments.
    """

    def __init__(self, symbol, args):
        self.symbol = symbol
        self.args = tuple(int(a) if is_int(a) else a for a in args)

    def __hash__(self):
        return hash((self.symbol, self.args))

    def __eq__(self, other):
        return (self.symbol, self.args) == (other.symbol, other.args)

    def __str__(self):
        return '{}{}'.format(self.symbol, "(" + ', '.join(map(str, self.args)) + ")")

    __repr__ = __str__

    def ground(self, binding):
        args = []
        for a in self.args:
            if is_action_parameter(a):
                args.append(binding[a])
            else:  # Assume it is a constant
                args.append(a)
        return Variable(self.symbol, args)


class ProblemObject(object):
    def __init__(self, name, typename):
        self.name = name
        self.typename = typename


class ObjectType(object):
    def __init__(self, name, parent):
        self.name = name
        self.parent = parent


class Symbol(object):
    def __init__(self, name, arguments):
        self.name = name
        self.arguments = arguments


class Predicate(Symbol):
    def __init__(self, name, domain):
        super().__init__(name, domain)
        self.codomain = '_bool_'


class Function(Symbol):
    def __init__(self, name, domain, codomain):
        super().__init__(name, domain)
        self.codomain = codomain


class FunctionInstantiation(object):
    def __init__(self, symbol, mapping=None):
        self.symbol = symbol
        self.mapping = mapping if mapping else {}

    def add(self, point, value):
        assert not point in self.mapping
        self.mapping[point] = value


class PredicateInstantiation(object):
    def __init__(self, symbol, mapping=None):
        self.symbol = symbol
        self.set = mapping if mapping else set()

    def add(self, point):
        assert not point in self.set
        self.set.add(point)


class Parameter(object):
    def __init__(self, name, typename):
        self.name = name
        self.typename = typename

    def __str__(self):
        return '{}: {}'.format(self.name, self.typename)


class State(object):
    def __init__(self, instantiations):
        self.instantiations = instantiations


class ProblemDomain(object):
    def __init__(self, name, types, symbols):
        self.name = name
        self.types = types
        self.symbols = self.index_by_name(symbols)

    def index_by_name(self, objects):
        """ Index the given objects by their name """
        ordered = OrderedDict()
        for obj in objects:
            ordered[obj.name] = obj
        return ordered

    def get_predicates(self):
        """ Small helper to iterate through the predicates """
        return (s for s in self.symbols.values() if isinstance(s, Predicate))

    def is_predicative(self):
        """ A domain is predicative if it has no functions, only predicates"""
        return all(isinstance(s, Predicate) for s in self.symbols.values())


class ProblemInstance(object):
    def __init__(self, name, domain, objects, init, static_data):
        self.name = name
        self.domain = domain
        self.objects = objects
        self.init = init
        self.static_data = static_data

    def get_complete_name(self):
        return self.domain.name + '/' + self.name


class Expression(object):
    def __init__(self, symbol, arguments=None):
        self.symbol = symbol
        self.arguments = arguments if arguments else []

    def __str__(self):
        args = "" if self.arguments is None else "({})".format(', '.join(map(str, self.arguments)))
        return "{}{}".format(self.symbol, args)

    def is_fluent(self):
        return not self.is_static()

    def is_static(self):
        return False  # By default, expressions are not static

    def is_subtree_static(self, arguments=None):
        arguments = (self.arguments if self.arguments is not None else []) if arguments is None else arguments
        for arg in arguments:
            if isinstance(arg, (FunctionalExpression, VariableExpression)):
                if not arg.is_static() or not self.is_subtree_static(arg.arguments):
                    return False
        return True

    def is_tree_static(self):
        return self.is_static() and self.is_subtree_static()

    def dump(self, object_index, parameter_index):
        raise RuntimeError("Needs to be subclassed")


class FunctionalExpression(Expression):
    def dump(self, objects, parameters):
        subterms = [elem.dump(objects, parameters) for elem in self.arguments]
        return dict(type='nested', symbol=self.symbol, subterms=subterms)


class StaticFunctionalExpression(FunctionalExpression):
    def is_static(self):
        return True


class PredicativeExpression(Expression):
    def __init__(self, symbol, negated, arguments=None):
        super().__init__(symbol, arguments)
        self.negated = negated

    def __str__(self):
        p = Expression.__str__(self)
        return '{}{}'.format("not " if self.negated else "", p)

    def dump(self, objects, parameters):
        assert len(self.arguments) == 2
        subterms = [elem.dump(objects, parameters) for elem in self.arguments]
        return dict(type='atom', symbol=self.process_symbol(), subterms=subterms)

    def process_symbol(self):
        return _process_symbol(self.symbol, self.negated)


class StaticPredicativeExpression(PredicativeExpression):
    def is_static(self):
        return True


class RelationalExpression(StaticPredicativeExpression):
    def __init__(self, symbol, negated, arguments):
        assert len(arguments) == 2
        super().__init__(symbol, negated, arguments)


class ArithmeticExpression(StaticFunctionalExpression):
    def __init__(self, symbol, arguments):
        assert len(arguments) == 2
        super().__init__(symbol, arguments)


class VariableExpression(Expression):
    def __init__(self, variable):
        assert isinstance(variable, Variable)
        super().__init__(variable.symbol, None)
        self.variable = variable

    def __str__(self):
        return str(self.variable)

    def dump(self, objects, parameters):
        raise RuntimeError("dump() should never be called on VariableExpressions!")


class ParameterExpression(Expression):
    def __init__(self, name):
        super().__init__(name)

    def dump(self, objects, parameters):
        return dict(type='parameter', position=parameters[self.symbol], name=self.symbol)


class ObjectExpression(Expression):
    def dump(self, objects, parameters):
        return dict(type='constant', value=objects.get_index(self.symbol))


class NumericExpression(Expression):
    def dump(self, objects, parameters):
        return dict(type='int_constant', value=int(self.symbol))


_NEGATED_SYMBOLS = {"=": "!=", "<": ">=", "<=": ">", ">": "<=", ">=": "<"}


def _process_symbol(symbol, negated):
    if not negated:
        return symbol
    return _NEGATED_SYMBOLS[symbol]
