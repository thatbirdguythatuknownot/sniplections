#include <stdio.h>
#include <math.h>
#include <string.h>

typedef double number;

typedef struct {
    int paren;
    const char *pos;
    int has; /* parser indicator */
} Parser;

typedef enum {
    P_EXPR,
    P_BOP1,
    P_BOP2,
    P_UOP,
    P_BOP3,
    P_ATOM,
} Prec;

#define IS_WSP(x) \
    (((x) == ' ') || ((x) == '\t') || \
     ((x) == '\r') || ((x) == '\n'))

#define SKIP_WSP(parser) \
    if (IS_WSP(*(parser)->pos)) { \
        do { \
            ++(parser)->pos; \
        } while (IS_WSP(*(parser)->pos)); \
    } \

#define ERR_PROP(res, parser) \
    if ((res) == -1. && (parser)->has) { \
        return -1.; \
    } \

#define RET_IF(cond, val) \
    if ((cond)) { \
        return val; \
    } \

number parse_expr(Parser *parser, Prec prec, int ret_immd) {
    number res, rhs;
    char c;
    int length;

    SKIP_WSP(parser)

    switch (*parser->pos) {
    case '-':
        ++parser->pos;
        res = parse_expr(parser, P_UOP, 0);
        ERR_PROP(res, parser)
        res = -res;
        break;
    case '+':
        ++parser->pos;
        res = parse_expr(parser, P_UOP, 0);
        ERR_PROP(res, parser)
        res = +res;
        break;
    case '(':
        ++parser->pos;
        parser->paren++;
        res = parse_expr(parser, P_EXPR, 0);
        ERR_PROP(res, parser)
        if (*parser->pos == ')') {
            ++parser->pos;
            if (--parser->paren < 0) {
                fputs("WARNING: non-matching closing parenthesis\n", stderr);
                parser->paren = 0;
            }
        }
        else {
            fputs("WARNING: unclosed parenthesis\n", stderr);
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
    RET_IF(ret_immd, res)

    for (; *parser->pos;) {
        SKIP_WSP(parser)
        switch (*parser->pos) {
        case '-':
            RET_IF(prec >= P_BOP1, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_BOP1, 0);
            ERR_PROP(rhs, parser)
            res -= rhs;
            break;
        case '+':
            RET_IF(prec >= P_BOP1, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_BOP1, 0);
            ERR_PROP(rhs, parser)
            res += rhs;
            break;
        case '*':
            RET_IF(prec >= P_BOP2, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_BOP2, 0);
            ERR_PROP(rhs, parser)
            res *= rhs;
            break;
        case '/':
            RET_IF(prec >= P_BOP2, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_BOP2, 0);
            ERR_PROP(rhs, parser)
            res /= rhs;
            break;
        case '%':
            RET_IF(prec >= P_BOP2, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_BOP2, 0);
            ERR_PROP(rhs, parser)
            res = fmod(res, rhs);
            break;
        case '^':
            RET_IF(prec > P_BOP3, res)
            ++parser->pos;
            rhs = parse_expr(parser, P_UOP, 0);
            ERR_PROP(rhs, parser)
            res = pow(res, rhs);
            break;
        case ')':
            return res;
        case '\0':
            break;
        default:
            fputs("ERROR: unknown character encountered", stderr);
            parser->has = 1;
            return -1.;
        }
    }

    return res;
}

int main() {
    char buffer[4096] = {0};
    const char *begin = &buffer[0];
    int length;
    Parser parser = {0, begin, 0};

    puts(
        "Calculator (type 'Q' or 'q' to quit)\n"
        "Supported operations:\n"
        "   addition (+)\n"
        "   subtraction (-)\n"
        "   multiplication (*)\n"
        "   division (/)\n"
        "   modulo (%)\n"
        "   power (^)\n"
    );

    while (scanf("%4095[^\n]%n ", buffer, &length) == 1) {
        if (length == 1 && (buffer[0] == 'Q' || buffer[0] == 'q')) {
            break;
        }
        number res = parse_expr(&parser, P_EXPR, 0);
        if (res == -1. && parser.has) {
            fprintf(stderr, " (position %ld)\n", parser.pos - begin);
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
