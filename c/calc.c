#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

typedef double number;

typedef struct {
    char paren_stack[200];
    int paren;
    const char *pos;
    int has; /* parser indicator */
} Parser;

typedef enum {
    P_EXPR,
    P_BOP1,
    P_BOP2,
    P_JUXT,
    P_UOP,
    P_BOP3,
    P_ATOM,
} Prec;

#define IS_WSP(x) \
    ((x) == ' ' || (x) == '\t' || \
     (x) == '\r' || (x) == '\n')

#define SKIP_WSP(parser) \
    if (IS_WSP(*(parser)->pos)) { \
        do { \
            ++(parser)->pos; \
        } while (IS_WSP(*(parser)->pos)); \
    } \

#define ERR_PROP(parser) \
    if ((parser)->has) { \
        return -1.; \
    } \

#define RET_IF(cond, val) \
    if ((cond)) { \
        return val; \
    } \

#define PFLIP(c) \
    (c == '[' ? ']' : ')')

#define RPNAME(c) \
    (c == ']' ? "bracket" : "parenthesis")

number parse_expr(Parser *parser, Prec prec) {
    number res, rhs;
    char c;
    int length;

    SKIP_WSP(parser)

    switch (*parser->pos) {
    case '-':
        ++parser->pos;
        res = parse_expr(parser, P_UOP);
        ERR_PROP(parser)
        res = -res;
        break;
    case '+':
        ++parser->pos;
        res = parse_expr(parser, P_UOP);
        ERR_PROP(parser)
        res = +res;
        break;
    case '(':
    case '[':
        c = *parser->pos++;
        parser->paren_stack[parser->paren++] = c;
        c = PFLIP(c);
        res = parse_expr(parser, P_EXPR);
        ERR_PROP(parser)
        if (*parser->pos == c) {
            ++parser->pos;
            if (--parser->paren < 0) {
                fprintf(stderr, "WARNING: non-matching closing %s\n", RPNAME(c));
                parser->paren = 0;
            }
        }
        else {
            fprintf(stderr, "WARNING: unclosed %s\n", RPNAME(c));
        }
        break;
    default:
        if (sscanf(parser->pos, "%lf%n", &res, &length) == 1) {
            parser->pos += length;
            break;
        }
        parser->has = 1;
        fputs("ERROR: expected number", stderr);
        return -1;
    }

    for (; *parser->pos;) {
        SKIP_WSP(parser)
        switch ((c = *parser->pos)) {
        case '-':
            RET_IF(prec >= P_BOP1, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_BOP1);
            ERR_PROP(parser)
            res -= rhs;
            break;
        case '+':
            RET_IF(prec >= P_BOP1, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_BOP1);
            ERR_PROP(parser)
            res += rhs;
            break;
        case '*':
            RET_IF(prec >= P_BOP2, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_BOP2);
            ERR_PROP(parser)
            res *= rhs;
            break;
        case '/':
            RET_IF(prec >= P_BOP2, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_BOP2);
            ERR_PROP(parser)
            res /= rhs;
            break;
        case '%':
            RET_IF(prec >= P_BOP2, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_BOP2);
            ERR_PROP(parser)
            res = fmod(res, rhs);
            break;
        case '^':
            RET_IF(prec > P_BOP3, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_UOP);
            ERR_PROP(parser)
            res = pow(res, rhs);
            break;
        case ')':
        case ']':
            RET_IF(parser->paren > 0 &&
                       PFLIP(parser->paren_stack[parser->paren - 1]) == c ||
                   prec != P_EXPR, res)
            if (parser->paren <= 0) {
                parser->paren = 0;
            }
            fprintf(stderr, "WARNING: unopened closing %s\n", RPNAME(c));
            ++parser->pos;
            break;
        case '(':
        case '[':
            RET_IF(prec >= P_JUXT, res)
            ++parser->pos;
            parser->paren_stack[parser->paren++] = c;
            c = PFLIP(c);
            rhs = parse_expr(parser, P_EXPR);
            ERR_PROP(parser)
            if (*parser->pos == c) {
                ++parser->pos;
                if (--parser->paren < 0) {
                    fprintf(stderr, "WARNING: non-matching closing %s\n", RPNAME(c));
                    parser->paren = 0;
                }
            }
            else {
                fprintf(stderr, "WARNING: unclosed %s\n", RPNAME(c));
            }
            res *= rhs;
            break;
        case '\0':
            break;
        default:
            RET_IF(prec >= P_JUXT, res)
            rhs = parse_expr(parser, P_JUXT);
            ERR_PROP(parser)
            res *= rhs;
            break;
        }
    }

    return res;
}

int main() {
    char buffer[4096] = {0};
    const char *begin = &buffer[0];
    int length;
    Parser parser = { .paren = 0,
                      .pos = begin,
                      .has = 0 };

    puts(
        "Calculator (type 'Q' or 'q' to quit)\n"
        "Supported operations:\n"
        "   addition (+)\n"
        "   subtraction (-)\n"
        "   multiplication (*)\n"
        "   division (/)\n"
        "   modulo (%)\n"
        "   negation (unary -)\n"
        "   juxtaposition (e.g. 2(3 + 4) )\n"
        "   power (^)\n"
    );

    while (scanf(" %4095[^\r\n]%n", buffer, &length) == 1) {
        if (length == 1 && (buffer[0] == 'Q' || buffer[0] == 'q')) {
            break;
        }
        number res = parse_expr(&parser, P_EXPR);
        if (parser.has) {
            fprintf(stderr, " (position %lld)\n", parser.pos - begin);
            parser.has = 0;
            goto cleanup;
        }
        printf("Result: %g\n", res);
      cleanup:
        parser.paren = 0;
        parser.pos = begin;
        memset(buffer, 0, 4095);
    }
  quitted:
    puts("quitted");
}
