#pragma once

{%- for ex in type_info.extensions %}
#include "{{ex|ext.type}}_cpp.h"
{%- endfor %}

{%- set req_attrs = type_info|class.req_attrs -%}
{%- set opt_attrs = type_info|class.opt_attrs -%}
{%- set ordered_attrs = req_attrs + opt_attrs -%}

{% for include in ordered_attrs|map('cpp.includes', False, True)|sum(start=[])|sort|unique %}
#include {{include|incl.quote_fix(path_package)}}
{%- endfor %}

#include "{{type_name}}.h"