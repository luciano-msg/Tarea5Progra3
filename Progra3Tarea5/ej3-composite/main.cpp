#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <cmath>

class Expression;
using ExprPtr = std::shared_ptr<Expression>;

class Expression {
public:
    virtual ~Expression() = default;
    virtual double evaluate(double x) const = 0;
    virtual ExprPtr derivative() const = 0;
    virtual std::string toString() const = 0;
    // Precedencia para decidir paréntesis en toString
    virtual int precedence() const { return 4; }
    // true si la subexpresión no depende de x (permite simplificar sin dynamic_cast)
    virtual bool isConstant() const = 0;
};

// ---------- Hojas ----------
class Constant : public Expression {
    double value;
public:
    explicit Constant(double v) : value(v) {}
    double evaluate(double) const override { return value; }
    ExprPtr derivative() const override { return std::make_shared<Constant>(0); }
    std::string toString() const override {
        std::ostringstream os;
        os << value;
        return os.str();
    }
    int precedence() const override { return value < 0 ? 1 : 4; }
    bool isConstant() const override { return true; }
};

class Variable : public Expression {
    std::string name;
public:
    explicit Variable(std::string n = "x") : name(std::move(n)) {}
    double evaluate(double x) const override { return x; }
    ExprPtr derivative() const override { return std::make_shared<Constant>(1); }
    std::string toString() const override { return name; }
    bool isConstant() const override { return false; }
};

// ---------- Helpers de construcción con simplificación básica ----------
ExprPtr makeAdd(ExprPtr a, ExprPtr b);
ExprPtr makeSub(ExprPtr a, ExprPtr b);
ExprPtr makeMul(ExprPtr a, ExprPtr b);
ExprPtr makeDiv(ExprPtr a, ExprPtr b);
ExprPtr makePow(ExprPtr a, ExprPtr b);
ExprPtr makeNeg(ExprPtr a);
ExprPtr makeLn(ExprPtr a);

bool isValue(const ExprPtr& e, double v) {
    return e->isConstant() && e->evaluate(0) == v;
}

ExprPtr fold(const ExprPtr& e) {
    return e->isConstant() ? std::make_shared<Constant>(e->evaluate(0)) : e;
}

// ---------- Compuestos binarios ----------
class BinaryOperation : public Expression {
protected:
    ExprPtr left, right;
    std::string symbol;
    int prec;
    bool wrapRightOnEqual;   // a - (b - c), a / (b * c)
    bool wrapLeftOnEqual;    // (a ^ b) ^ c

    std::string wrap(const ExprPtr& e, bool onEqual) const {
        int p = e->precedence();
        bool need = onEqual ? p <= prec : p < prec;
        return need ? "(" + e->toString() + ")" : e->toString();
    }
public:
    BinaryOperation(ExprPtr l, ExprPtr r, std::string sym, int p,
                    bool wrapRight = false, bool wrapLeft = false)
        : left(std::move(l)), right(std::move(r)), symbol(std::move(sym)),
          prec(p), wrapRightOnEqual(wrapRight), wrapLeftOnEqual(wrapLeft) {}

    std::string toString() const override {
        return wrap(left, wrapLeftOnEqual) + symbol + wrap(right, wrapRightOnEqual);
    }
    int precedence() const override { return prec; }
    bool isConstant() const override { return left->isConstant() && right->isConstant(); }
};

class Add : public BinaryOperation {
public:
    Add(ExprPtr l, ExprPtr r) : BinaryOperation(std::move(l), std::move(r), " + ", 1) {}
    double evaluate(double x) const override { return left->evaluate(x) + right->evaluate(x); }
    ExprPtr derivative() const override {
        return makeAdd(left->derivative(), right->derivative());
    }
};

class Subtract : public BinaryOperation {
public:
    Subtract(ExprPtr l, ExprPtr r) : BinaryOperation(std::move(l), std::move(r), " - ", 1, true) {}
    double evaluate(double x) const override { return left->evaluate(x) - right->evaluate(x); }
    ExprPtr derivative() const override {
        return makeSub(left->derivative(), right->derivative());
    }
};

class Multiply : public BinaryOperation {
public:
    Multiply(ExprPtr l, ExprPtr r) : BinaryOperation(std::move(l), std::move(r), " * ", 2) {}
    double evaluate(double x) const override { return left->evaluate(x) * right->evaluate(x); }
    // (f g)' = f' g + f g'
    ExprPtr derivative() const override {
        return makeAdd(makeMul(left->derivative(), right),
                       makeMul(left, right->derivative()));
    }
};

class Divide : public BinaryOperation {
public:
    Divide(ExprPtr l, ExprPtr r) : BinaryOperation(std::move(l), std::move(r), " / ", 2, true) {}
    double evaluate(double x) const override { return left->evaluate(x) / right->evaluate(x); }
    // (f / g)' = (f' g - f g') / g^2
    ExprPtr derivative() const override {
        return makeDiv(makeSub(makeMul(left->derivative(), right),
                               makeMul(left, right->derivative())),
                       makePow(right, std::make_shared<Constant>(2)));
    }
};

class Power : public BinaryOperation {
public:
    Power(ExprPtr base, ExprPtr exponent)
        : BinaryOperation(std::move(base), std::move(exponent), "^", 3, false, true) {}
    double evaluate(double x) const override { return std::pow(left->evaluate(x), right->evaluate(x)); }
    ExprPtr derivative() const override {
        // f^n  ->  n * f^(n-1) * f'
        if (right->isConstant()) {
            auto n = fold(right);
            return makeMul(makeMul(n, makePow(left, makeSub(n, std::make_shared<Constant>(1)))),
                           left->derivative());
        }
        // a^g  ->  a^g * ln(a) * g'
        if (left->isConstant()) {
            return makeMul(makeMul(makePow(left, right), makeLn(left)), right->derivative());
        }
        // f^g  ->  f^g * (g' ln f + g f' / f)
        return makeMul(makePow(left, right),
                       makeAdd(makeMul(right->derivative(), makeLn(left)),
                               makeDiv(makeMul(right, left->derivative()), left)));
    }
};

// ---------- Compuestos unarios ----------
class UnaryFunction : public Expression {
protected:
    ExprPtr arg;
    std::string name;
    virtual double apply(double v) const = 0;
    // Derivada de la función externa evaluada en el argumento: g'(u)
    virtual ExprPtr outerDerivative() const = 0;
public:
    UnaryFunction(ExprPtr a, std::string n) : arg(std::move(a)), name(std::move(n)) {}
    double evaluate(double x) const override { return apply(arg->evaluate(x)); }
    // Regla de la cadena: g(u)' = g'(u) * u'
    ExprPtr derivative() const override { return makeMul(outerDerivative(), arg->derivative()); }
    std::string toString() const override { return name + "(" + arg->toString() + ")"; }
    bool isConstant() const override { return arg->isConstant(); }
};

class Sin : public UnaryFunction {
protected:
    double apply(double v) const override { return std::sin(v); }
    ExprPtr outerDerivative() const override;
public:
    explicit Sin(ExprPtr a) : UnaryFunction(std::move(a), "sin") {}
};

class Cos : public UnaryFunction {
protected:
    double apply(double v) const override { return std::cos(v); }
    ExprPtr outerDerivative() const override { return makeNeg(std::make_shared<Sin>(arg)); }
public:
    explicit Cos(ExprPtr a) : UnaryFunction(std::move(a), "cos") {}
};

ExprPtr Sin::outerDerivative() const { return std::make_shared<Cos>(arg); }

class Exp : public UnaryFunction {
protected:
    double apply(double v) const override { return std::exp(v); }
    ExprPtr outerDerivative() const override { return std::make_shared<Exp>(arg); }
public:
    explicit Exp(ExprPtr a) : UnaryFunction(std::move(a), "exp") {}
};

class Ln : public UnaryFunction {
protected:
    double apply(double v) const override { return std::log(v); }
    ExprPtr outerDerivative() const override { return makeDiv(std::make_shared<Constant>(1), arg); }
public:
    explicit Ln(ExprPtr a) : UnaryFunction(std::move(a), "ln") {}
};

class Negate : public UnaryFunction {
protected:
    double apply(double v) const override { return -v; }
    ExprPtr outerDerivative() const override { return std::make_shared<Constant>(-1); }
public:
    explicit Negate(ExprPtr a) : UnaryFunction(std::move(a), "-") {}
    ExprPtr derivative() const override { return makeNeg(arg->derivative()); }
    std::string toString() const override {
        return arg->precedence() < 2 ? "-(" + arg->toString() + ")" : "-" + arg->toString();
    }
    int precedence() const override { return 1; }
};

// ---------- Implementación de helpers ----------
ExprPtr makeAdd(ExprPtr a, ExprPtr b) {
    if (isValue(a, 0)) return b;
    if (isValue(b, 0)) return a;
    return fold(std::make_shared<Add>(a, b));
}

ExprPtr makeSub(ExprPtr a, ExprPtr b) {
    if (isValue(b, 0)) return a;
    if (isValue(a, 0)) return makeNeg(b);
    return fold(std::make_shared<Subtract>(a, b));
}

ExprPtr makeMul(ExprPtr a, ExprPtr b) {
    if (isValue(a, 0) || isValue(b, 0)) return std::make_shared<Constant>(0);
    if (isValue(a, 1)) return b;
    if (isValue(b, 1)) return a;
    if (isValue(a, -1)) return makeNeg(b);
    if (isValue(b, -1)) return makeNeg(a);
    return fold(std::make_shared<Multiply>(a, b));
}

ExprPtr makeDiv(ExprPtr a, ExprPtr b) {
    if (isValue(a, 0)) return std::make_shared<Constant>(0);
    if (isValue(b, 1)) return a;
    return fold(std::make_shared<Divide>(a, b));
}

ExprPtr makePow(ExprPtr a, ExprPtr b) {
    if (isValue(b, 0)) return std::make_shared<Constant>(1);
    if (isValue(b, 1)) return a;
    return fold(std::make_shared<Power>(a, b));
}

ExprPtr makeNeg(ExprPtr a) {
    if (a->isConstant()) return std::make_shared<Constant>(-a->evaluate(0));
    return std::make_shared<Negate>(a);
}

ExprPtr makeLn(ExprPtr a) {
    return fold(std::make_shared<Ln>(a));
}

// Verificación numérica de la derivada simbólica
void check(const std::string& label, const ExprPtr& f, double x0) {
    const double h = 1e-6;
    double numeric = (f->evaluate(x0 + h) - f->evaluate(x0 - h)) / (2 * h);
    double symbolic = f->derivative()->evaluate(x0);
    std::cout << label << "\n  f(x)  = " << f->toString()
              << "\n  f'(x) = " << f->derivative()->toString()
              << "\n  f'(" << x0 << ") simbolica = " << symbolic
              << " | numerica = " << numeric
              << (std::fabs(symbolic - numeric) < 1e-4 ? "  OK" : "  ERROR") << "\n\n";
}

int main() {
    auto x = std::make_shared<Variable>();

    auto expr =
        std::make_shared<Multiply>(
            std::make_shared<Add>(
                std::make_shared<Power>(
                    x,
                    std::make_shared<Constant>(2)
                ),
                std::make_shared<Multiply>(
                    std::make_shared<Constant>(3),
                    x
                )
            ),
            std::make_shared<Sin>(x)
        );

    auto deriv = expr->derivative();

    std::cout << "f(x) = " << expr->toString() << std::endl;
    std::cout << "f'(x) = " << deriv->toString() << std::endl;

    std::cout << "f(2) = " << expr->evaluate(2) << std::endl;
    std::cout << "f'(2) = " << deriv->evaluate(2) << std::endl;
    std::cout << "f''(x) = " << deriv->derivative()->toString() << "\n\n";

    std::cout << "--- Verificacion con diferencias finitas ---\n";
    check("Expresion del enunciado", expr, 2);
    check("sin(x^2) (regla de la cadena)",
          std::make_shared<Sin>(std::make_shared<Power>(x, std::make_shared<Constant>(2))), 1.3);
    check("exp(cos(x)) / x (cociente + anidamiento)",
          std::make_shared<Divide>(std::make_shared<Exp>(std::make_shared<Cos>(x)), x), 0.7);
    check("x^x (potencia general)",
          std::make_shared<Power>(x, x), 1.5);
    check("2^(3x) - ln(x)",
          std::make_shared<Subtract>(
              std::make_shared<Power>(std::make_shared<Constant>(2),
                                      std::make_shared<Multiply>(std::make_shared<Constant>(3), x)),
              std::make_shared<Ln>(x)), 0.5);
}
