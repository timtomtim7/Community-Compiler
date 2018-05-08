//
// Created by myvar on 5/6/18.
//

#include "Semantics.h"
#include "Ast.h"

void Semantics::p1_fn(AstFn *node) {
    p1_funcs.push_back(node->name);
}

void Semantics::p1_struct(AstStruct *node) {
    p1_structs.push_back(node->name);
}

void Semantics::pass1_node(AstNode *node) {
    switch(node->node_type) {
    case AstNodeType::AstFn:
        p1_fn((AstFn *)node);
        break;
    case AstNodeType::AstAffix: {
        auto x = (AstAffix *)node;
        p1_funcs.push_back(x->name);

    } break;
    case AstNodeType::AstStruct:
        p1_struct((AstStruct *)node);
        break;
    case AstNodeType::AstBlock:
        for(auto x : ((AstBlock *)node)->statements) {
            pass1_node(x);
        }
        break;
    case AstNodeType::AstImpl: {
        for(auto s : ((AstImpl *)node)->block->statements) {
            if(s->node_type == AstNodeType::AstFn) {
                AstSymbol *p            = new AstSymbol();
                p->name                 = ((AstImpl *)node)->name->name;
                ((AstFn *)s)->type_self = p;
            }
        }

        pass1_node(((AstImpl *)node)->block);
    } break;
    case AstNodeType::AstExtern: {
        for(auto x : ((AstExtern *)node)->decls) {
            pass1_node(x);
        }
    } break;
    }
}

void Semantics::pass1(Ast &ast) {
    pass1_node(ast.root);
    for(auto x : p1_funcs) {
        // printf("%s\n", x->name.c_str());
    }
}

void Semantics::pass2(Ast &ast) {
    pass2_node(ast.root);
}

void Semantics::pass2_node(AstNode *node) {
    switch(node->node_type) {
    case AstNodeType::AstFn:
        p2_fn((AstFn *)node);
        break;
    case AstNodeType::AstAffix: {
        auto x = (AstAffix *)node;
        p2_affix((AstAffix *)node);
    } break;
    case AstNodeType::AstStruct:
        p2_struct((AstStruct *)node);
        break;
    case AstNodeType::AstBlock:
        for(auto x : ((AstBlock *)node)->statements) {
            pass2_node(x);
        }
        break;
    case AstNodeType::AstImpl: {
        pass2_node(((AstImpl *)node)->block);
    } break;
    case AstNodeType::AstExtern: {
        for(auto x : ((AstExtern *)node)->decls) {
            pass2_node(x);
        }
    } break;
    }
}

static std::string type_to_string(const AstType *node) {
    if(node->is_array) {
        return type_to_string(node->subtype) + "Arr";
    }
    return node->name;
}

void Semantics::p2_affix(AstAffix *node) {
    if(node->return_type != nullptr) {
        if(!p1_hasSymbol(node->return_type)) {
            printf(
                "The type \"%s\" does not exist\n",
                node->return_type->name.c_str());
            return;
        }
    }

    for(auto a : node->params) {
        if(a->type != nullptr) {
            if(!p1_hasSymbol(a->type)) {
                printf(
                    "The type \"%s\" does not exist\n", a->type->name.c_str());
                return;
            }
        }
    }
    if(node->body != nullptr) {
        for(auto a : node->params) {
            // node->name->name += type_to_string(a->type);
        }
    }
    p2_affixs.push_back(node);
}

void Semantics::p2_fn(AstFn *node) {
    if(node->type_self != nullptr) {
        if(!p1_hasSymbol(node->type_self)) {
            printf(
                "The type \"%s\" does not exist\n",
                node->type_self->name.c_str());
            return;
        }

        // we need to add implicit self
        AstDec *self     = new AstDec();
        self->name       = new AstSymbol();
        self->name->name = "self";

        self->type       = new AstType();
        self->type->name = node->type_self->name;

        node->params.insert(node->params.begin(), self);
    }

    if(node->return_type != nullptr) {
        if(!p1_hasSymbol(node->return_type)) {
            printf(
                "The type \"%s\" does not exist\n",
                node->return_type->name.c_str());
            return;
        }
    }

    for(auto a : node->params) {
        if(a->type != nullptr) {
            if(!p1_hasSymbol(a->type)) {
                printf(
                    "The type \"%s\" does not exist\n", a->type->name.c_str());
                return;
            }
        }
    }
    if(node->body != nullptr) {
        for(auto a : node->params) {
            // node->name->name += type_to_string(a->type);
        }
    }
    p2_funcs.push_back(node);
}

void Semantics::p2_struct(AstStruct *node) {
    for(auto b : node->block->statements) {
        if(b->node_type == AstNodeType::AstDec) {
            if(((AstDec *)b)->type != nullptr) {
                if(!p1_hasSymbol(((AstDec *)b)->type)) {
                    printf(
                        "The type \"%s\" does not exist\n",
                        ((AstDec *)b)->type->name.c_str());
                    return;
                }
            }
        }
    }
}

/*
 * expretion type cheacking
 * flat structs
 * inlineing
 * inline assembly
 *
 * lost exprestion is rteurn type move it into a return ast
 */
void Semantics::pass3(Ast &ast) {
    pass3_nest_att(ast.root);
    pass3_node(ast.root);
}

void Semantics::pass3_nest_att(AstNode *node) {
    if(node->node_type == AstNodeType::AstAttribute) {
        nest_flag = true;
        nested_attributes.push_back((AstAttribute *)node);
    } else {
        if(nest_flag) {
            nest_flag = false;

            for(auto e : nested_attributes) {
                node->nested_attributes.push_back(e);
            }
            nested_attributes.clear();
        }
    }

    switch(node->node_type) {
    case AstNodeType::AstBlock: {
        auto x = (AstBlock *)node;
        for(auto e : x->statements) {
            pass3_nest_att(e);
        }
        break;
    }

    case AstNodeType::AstStruct: {
        auto x = (AstStruct *)node;
        for(auto e : x->block->statements) {
            pass3_nest_att(e);
        }
        break;
    }
    case AstNodeType::AstImpl: {
        auto x = (AstImpl *)node;
        for(auto e : x->block->statements) {
            pass3_nest_att(e);
        }
        break;
    }
    }
}

void Semantics::pass3_node(AstNode *node) {
    for(auto y : node->nested_attributes) {
        if(y->name->name == "il") {
            node->emit = false;
        }
    }

    switch(node->node_type) {
    case AstNodeType::AstBlock: {
        auto x = (AstBlock *)node;

        for(auto i : x->statements) {
            pass3_node(i);
            i = inline_if_need_be(i);
        }
        break;
    }
    case AstNodeType::AstString: {
        auto x = (AstString *)node;

        break;
    }
    case AstNodeType::AstNumber: {
        auto x = (AstNumber *)node;

        break;
    }
    case AstNodeType::AstBoolean: {
        auto x = (AstBoolean *)node;

        break;
    }
    case AstNodeType::AstArray: {
        auto x = (AstArray *)node;

        break;
    }
    case AstNodeType::AstDec: {
        auto x = (AstDec *)node;
        pass3_node(x->value);
        x->value = inline_if_need_be(x->value);
        break;
    }
    case AstNodeType::AstIf: {
        auto x = (AstIf *)node;

        pass3_node(x->condition);
        x->condition = inline_if_need_be(x->condition);

        for(auto i : x->true_block->statements) {
            pass3_node(i);
            i = inline_if_need_be(i);
        }

        if(x->false_block != nullptr) {
            for(auto i : x->false_block->statements) {
                i = inline_if_need_be(i);
            }
        }
        break;
    }
    case AstNodeType::AstFn: {
        auto x = (AstFn *)node;

        for(auto a : p2_funcs) {
            if(a != x && a->name->name == x->name->name) {
                printf(
                    "Duplicite fn decleration found for \"%s\"\n",
                    a->name->name.c_str());
                return;
            }
        }
        if(x->body != nullptr) {
            for(auto i : x->body->statements) {
                i = inline_if_need_be(i);
                pass3_node(x->body);
            }
        }
        break;
    }
    case AstNodeType::AstFnCall: {
        auto x = (AstFnCall *)node;

        auto mn = p2_get_fn(x->name);
        if(mn->nested_attributes.size() == 0) {
            for(auto a : x->nested_attributes) {
                if(a->name->name == "inline") {
                    x->emit = false;
                }
            }
        }

        auto nm = p2_get_fn(x->name);
        if(nm != nullptr && nm->body != nullptr) {
            for(auto a : nm->params) {
                x->name->name += type_to_string(a->type);
            }
        }

        for(auto i : x->args) {
            i = inline_if_need_be(i);
            pass3_node(i);
        }

        break;
    }
    case AstNodeType::AstLoop: {
        auto x = (AstLoop *)node;

        pass3_node(x->expr);

        x->expr = inline_if_need_be(x->expr);

        for(auto i : x->body->statements) {
            i = inline_if_need_be(i);
        }

        break;
    }
    case AstNodeType::AstContinue: {
        auto x = (AstContinue *)node;

        break;
    }
    case AstNodeType::AstBreak: {
        auto x = (AstBreak *)node;

        break;
    }
    case AstNodeType::AstStruct: {
        auto x = (AstStruct *)node;

        break;
    }

    case AstNodeType::AstImpl: {
        auto x = (AstImpl *)node;

        for(auto i : x->block->statements) {
            pass3_node(i);
            i = inline_if_need_be(i);
        }

        break;
    }
    case AstNodeType::AstAttribute: {
        auto x = (AstAttribute *)node;

        break;
    }
    case AstNodeType::AstAffix: {
        auto x = (AstAffix *)node;

        p3_affix(x);

        for(auto i : x->body->statements) {
            pass3_node(i);
            i = inline_if_need_be(i);
        }
        break;
    }
    case AstNodeType::AstUnaryExpr: {
        auto x = (AstUnaryExpr *)node;

        {
            auto nm = p2_get_fn(x->op);
            if(nm != nullptr && nm->body != nullptr) {
                for(auto a : nm->params) {
                    x->op += type_to_string(a->type);
                }
            }
        }

        {
            auto nm = p2_get_affix(x->op);
            if(nm != nullptr && nm->body != nullptr) {
                for(auto a : nm->params) {
                    x->op += type_to_string(a->type);
                }
            }
        }

        pass3_node(x->expr);
        x->expr = inline_if_need_be(x->expr);
        break;
    }
    case AstNodeType::AstBinaryExpr: {
        auto x = (AstBinaryExpr *)node;

        {
            auto nm = p2_get_fn(x->op);
            if(nm != nullptr && nm->body != nullptr) {
                for(auto a : nm->params) {
                    x->op += type_to_string(a->type);
                }
            }
        }

        {
            auto nm = p2_get_affix(x->op);
            if(nm != nullptr && nm->body != nullptr) {
                for(auto a : nm->params) {
                    x->op += type_to_string(a->type);
                }
            }
        }

        pass3_node(x->lhs);
        pass3_node(x->rhs);

        x->lhs = inline_if_need_be(x->lhs);
        x->rhs = inline_if_need_be(x->rhs);
        break;
    }
    case AstNodeType::AstIndex: {
        auto x = (AstIndex *)node;

        break;
    }
    case AstNodeType::AstType: {
        auto x = (AstType *)node;

        break;
    }
    case AstNodeType::AstSymbol: {
        auto x = (AstSymbol *)node;

        break;
    }
    case AstNodeType::AstReturn: {
        auto x = (AstReturn *)node;
        pass3_node(x->expr);
        x->expr = inline_if_need_be(x->expr);
        break;
    }
    case AstNodeType::AstExtern: {
        auto x = (AstExtern *)node;

        for(auto i : x->decls) {
            pass3_node(i);
        }

        break;
    }
    }
}

void Semantics::p3_struct(AstStruct *node) {}

/*
 * prefex may onlyhave one arg
 * suffix may only have one arg
 * infix must have 2 args and any return type
 */
void Semantics::p3_affix(AstAffix *node) {
    switch(node->affix_type) {
    case AffixType::Prefix:
        if(node->params.size() != 1) {
            printf(
                "The Prefix \"%s\" must have only one parameter\n",
                node->name->name.c_str());
        }
        break;
    case AffixType::Suffix:
        if(node->params.size() != 1) {
            printf(
                "The Suffix \"%s\" must have only one parameter\n",
                node->name->name.c_str());
        }
        break;
    case AffixType::Infix:
        if(node->params.size() != 2) {
            printf(
                "The Infix \"%s\" must have only two parameters\n",
                node->name->name.c_str());
        }

        if(node->return_type == nullptr) {
            printf(
                "The Infix \"%s\" must have an return type\n",
                node->name->name.c_str());
        }
        break;
    }
}

AstNode *Semantics::inline_if_need_be(AstNode *node) {
    bool flag = false;

    if(node->nested_attributes.size() == 0) {
        for(auto a : node->nested_attributes) {
            if(a->name->name == "inline") {
                flag = true;
                break;
            }
        }
    }
    if(!flag || node->node_type != AstNodeType::AstFnCall) {
        return node;
    }

    auto x = (AstFnCall *)node;

    printf(x->name->name.c_str());

    return node;
}