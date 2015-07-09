"""
    Some methods to compile actions.
"""
from pddl import Truth, Effect
from pddl.effects import AssignmentEffect

import base
from .helper import get_formula_parts, index_list, assignment_code, finish_bool_code
from .printer import CppPrinter
from compilation.parser import Parser, DefinedExpression, StaticPredicativeExpression, RelationalExpression


class ProcedureIndex(object):
    def __init__(self, parameters, object_idx, relevant):
        self.parameters = parameters
        self.object_idx = object_idx
        self.relevant = relevant


class ActionCompiler(object):
    def __init__(self, task, action):
        self.task = task
        self.action = action
        self.parser = Parser(self.task)

    def generate_app_procedures(self):
        """  Generates the applicability procedures from the PDDL parser precondition list"""
        for part in get_formula_parts(self.action.precondition):
            procedure = self.generate_app_procedure(part)
            self.processed_action.add_applicability_procedure(procedure)

    def generate_eff_procedures(self):
        """  Generates the effect procedures from the PDDL parser effect list"""
        for effect in self.action.effects:
            assert isinstance(effect, Effect) and isinstance(effect.condition, Truth) and not effect.parameters
            procedure = self.generate_eff_procedure(effect.literal)
            self.processed_action.add_effect_procedure(procedure)

    def process(self):
        params = [base.Parameter(p.name, p.type) for p in self.action.parameters]
        self.processed_action = base.Action(self.action.name, params)

        self.generate_app_procedures()
        self.generate_eff_procedures()

        return self.processed_action

    def build_procedure_index(self, relevant):
        return ProcedureIndex(self.processed_action.parameter_map, self.task.index.objects, index_list(relevant))

    def generate_app_procedure(self, expression):
        exp, relevant = self.parser.process(expression)
        index = self.build_procedure_index(relevant)

        if exp.is_static() and exp.is_subtree_static():
            if isinstance(exp, DefinedExpression):
                procedure = base.StaticAppProcedure(base.DefinedRoutine(exp))
            elif isinstance(exp, RelationalExpression):
                procedure = base.StaticAppProcedure(base.StaticRelationalRoutine(exp.symbol, exp.negated, exp.arguments))
            elif isinstance(exp, StaticPredicativeExpression):
                procedure = base.StaticAppProcedure(
                    base.StaticPredicativeRoutine(exp.symbol, exp.negated, exp.arguments))
            else:
                raise RuntimeError("Unknown type of expression '{}'".format(exp))

        else:
            printer = CppPrinter(index)
            code = finish_bool_code(printer.print(exp))
            procedure = base.AppProcedure(str(exp), relevant, code, comment=str(exp))

            # Check if there is a builtin constraint available:
            procedure.builtin = self.process_builtin_constraint(procedure, index, printer, exp)

        return procedure

    def process_builtin_constraint(self, procedure, index, printer, expression):
        if isinstance(expression, (base.ConstraintExpression, base.EffectExpression)):
            return expression  # The builtin component is already instantiated
        return base.ConstraintExpressionCatalog.instantiate(expression, index, printer)

    def generate_eff_procedure(self, expression):
        if isinstance(expression, AssignmentEffect):  # A functional effect
            lhs, affected = self.parser.process(expression.lhs)
            rhs, relevant = self.parser.process(expression.rhs)

            index = self.build_procedure_index(relevant)
            printer = CppPrinter(index)
            rhs_code = printer.print(rhs)
            code = "return Atom(_affected, {});".format(rhs_code)
            procedure = base.EffProcedure(str(expression), relevant, affected, code)

            # Check if there is a builtin effect available:
            procedure.builtin = base.EffectExpressionCatalog.instantiate(procedure, index, printer, rhs)

        else:  # A predicative effect
            exp, affected = self.parser.process(expression)
            value = "0" if expression.negated else "1"
            relevant = []  # Predicative effects cannot have nested relevant variables
            code = "return Atom(_affected, {});".format(value)
            procedure = base.EffProcedure(str(expression), relevant, affected, code)
            procedure.builtin = base.ValueAssignmentEffectExpression([], affected, [value])

        return procedure
