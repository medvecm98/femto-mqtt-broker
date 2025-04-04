#include "topic_list.h"

/**
 * Provided topic in string form is tokenized using '/' as separator. All
 * allocations are handled. Need to be freed when destroying.
 * 
 * WARNING: Method is destructive w.r.t. topic string.
 * 
 * \param topic Topic string (destructive).
 * \param count_out Pointer to integer, will return number of tokens found.
 * 
 * \returns Array of strings containing topic in tokenized form.
 */
char**
tokenize_topic(char *topic, int *count_out) {
	char *temp = topic;
	int count = 1;
	while ((temp = strstr(temp, "/"))) {
		// count number of topics based on delimiters
		count++;
		temp++;
	}

	char **tokenized_topic = calloc(count, sizeof(char*));
	if (!tokenized_topic)
		err(1, "tokenize topic calloc tokenized_topic");
	temp = strtok(topic, "/");
	for (int i = 0; i < count; i++) {
		tokenized_topic[i] = calloc(strlen(temp) + 1, sizeof(char));
		strncpy(tokenized_topic[i], temp, strlen(temp));
		temp = strtok(NULL, "/");
	}

	*count_out = count;

	return tokenized_topic;
}

/**
 * Inserts new topic into list. Topic is tokenized from its string form, and
 * both forms are saved. QoS code can be inserted as well.
 * 
 * NOTE: Implementation only supports QoS level 0. Other levels are ignored.
 * 
 * \param list Linked list of topics.
 * \param topic_str Topic in string form, to be tokenized.
 * \param topic_len Topic length, wihout null terminator.
 * \param qos_code QoS quarantee for this topic.
 */
void
insert_topic(topics_t *list, char *topic_str, size_t topic_len, int qos_code) {
	topic_t *topic = calloc(1, sizeof(topic_t));
	if (!topic)
		err(1, "insert topic calloc topic");
	topic_t *head = list->head;

	if (head == NULL) {
		head = topic;
	}
	else {
		head->next = topic;
	}
	topic->next = NULL;
	// tokenize_topic is destructive... make copy
	char* topic_copy = calloc(strnlen(topic_str, topic_len) + 1, sizeof(char));
	if (!topic_copy)
		err(1, "insert topic calloc topic_copy");
	strncpy(topic_copy, topic_str, topic_len);
	topic->topic = topic_copy;
	topic->tokenized_topic = tokenize_topic(
		topic_str,
		&topic->topic_token_count
	);
	topic->qos_code = qos_code;

	if (list->back == NULL) {
		list->back = topic;
	}

	list->head = topic;
}

/**
 * Removes topic from topic linked list based on its string representation.
 * Allocated memory is freed as needed.
 * 
 * \param list Linked list of topics.
 * \param topic_str Topic in string form, to be removed.
 * \param topic_len Topic length, wihout null terminator.
 * 
 * \returns 1, if removal took place, 0 otherwise.
 */
int
remove_topic(topics_t *list, char *topic_str, size_t topic_len) {
	topic_t *prev_topic = NULL;
	for (topic_t *topic = list->back; topic != NULL; topic = topic->next) {
		if (strncmp(topic->topic, topic_str, topic_len) == 0) {
			free(topic->topic);
			for (int i = 0; i < topic->topic_token_count; i++) {
				free(topic->tokenized_topic[i]);
			}
			free(topic->tokenized_topic);
			if (prev_topic != NULL) {
				prev_topic->next = topic->next;
				if (topic == list->head)
					list->head = prev_topic;
			}
			else {
				list->back = topic->next;
				if (topic == list->head)
					list->head = NULL;
			}
			free(topic);
			return 1;
		}

		prev_topic = topic;
	}
	return 0;
}

/**
 * Finds topic in topic linked list.
 * 
 * \param list Linked list of topics.
 * \param topic_str Topic in string form, to be found.
 * 
 * \returns 1, if present, 0 otherwise.
 */
int
find_topic(topics_t *list, char *topic_str) {
	for (topic_t *topic = list->back; topic != NULL; topic = topic->next) {
		if (strcmp(topic->topic, topic_str) == 0)
			return 1;
	}
	return 0;
}

/**
 * Initializes new topic linked list.
 * 
 * \returns New topic linked list.
 */
topics_t *
create_topics_list(void) {
	topics_t *list = calloc(1, sizeof(topics_t));
	if (!list)
		err(1, "create topic list calloc list");
	list->back = NULL;
	list->head = NULL;
	return list;
}

/**
 * Frees allocated memory by topic linked list.
 * 
 * \param list Topic linked list to delete.
 */
void
delete_topics_list(topics_t *list) {
	topic_t *prev_topic = NULL;

	for (topic_t *topic = list->back; topic != NULL; topic = topic->next) {
		free(topic->topic);
		for (int i = 0; i < topic->topic_token_count; i++) {
			free(topic->tokenized_topic[i]);
		}
		free(topic->tokenized_topic);
		if (prev_topic)
			free(prev_topic);
		prev_topic = topic;
	}

	if (prev_topic)
		free(prev_topic);

	free(list);
}
