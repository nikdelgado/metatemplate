"""Stores some shared keys/maps for custom types and external class schema
placeholders.
"""

PLACEHOLDER_PREFIX = '{external_lib}'

CUSTOM_UUID_KEY_MAP = {
    "UniversallyUniqueIdentifierType": '{custom_schema}utils::UUID',
    "DurationType": '{custom_schema}utils::Duration',
    "TimeType": '{custom_schema}utils::TimePoint',
    "DateTimeType": '{custom_schema}utils::TimePoint',
    "duration": '{custom_schema}utils::Duration',
    "dateTime": '{custom_schema}utils::TimePoint',
    "datetime": '{custom_schema}utils::TimePoint',
}

CUSTOM_QNAME_INCLUDES = {
    '{custom_schema}utils::UUID': "<{path_api}/utils/UUID.h>",
    '{custom_schema}utils::Duration': "<{path_api}/utils/Clock.h>",
    '{custom_schema}utils::TimePoint': "<{path_api}/utils/Clock.h>",
}

CUSTOM_UTIL_INCLUDES = {
    '{custom_schema}utils::UUID': 'utils/UUID.h',
    '{custom_schema}utils::Duration': 'utils/Duration.h',
    '{custom_schema}utils::TimePoint': 'utils/Datetime.h',
}