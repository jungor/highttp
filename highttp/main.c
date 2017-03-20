#include <stdio.h>
#include <io.h>
#include <uv.h>

#define HANDLE_UV_ERR_IF_NEED(r, func_name) \
	if (r < 0) { \
		fprintf(stderr, "%s: %s\n", func_name, uv_strerror(r)); \
		getchar(); \
		exit(1); \
	}

static uv_loop_t *loop; // �����¼�ѭ��
static uv_tcp_t server;  // tcp�����handle

/**
 * \brief 
 * \param handle 
 * \param suggested_size 
 * \param buf 
 */
void alloc_cb(struct uv_handle_s *handle, size_t suggested_size, struct uv_buf_t* buf) {
	buf->base = malloc(suggested_size);
	buf->len = suggested_size;
}


void on_close(struct uv_handle_s* client) {
	printf("disconnected\n");
	// �ͷ�ʱ������ʱע�͵���δ֪��uv�Ƿ���Ҫ�������ͷ�client�ռ�
	//free(client);
}

void read_cb(struct uv_stream_s *client, ssize_t nread, const struct uv_buf_t* buf) {
	if (buf->base == NULL) return;
	if (nread >= 0) {
		// TODO: ����HTPP������
		_write(1, buf->base, nread);
	} else {
		if (nread == UV_EOF) {
			uv_close((struct uv_handle_s*)client, on_close);
		}
	}
	free(buf->base);
}

/**
 * \brief uv_listen�Ļص������������µ����ӽ���ʱ�ᱻ����
 * \param server_handle tcp�����handle
 * \param status δ֪
 */
void on_connection(struct uv_stream_s *server_handle, int status) {
	int r;
	printf("connected\n");
	struct uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);

	// ��������
	r = uv_accept(server_handle, (uv_stream_t*)client);
	HANDLE_UV_ERR_IF_NEED(r, "uv_accept");

	// ��ȡ����
	r = uv_read_start((uv_stream_t*)client, alloc_cb, read_cb);
	HANDLE_UV_ERR_IF_NEED(r, "uv_read_start");
}

int main(int argc, char* argv[]) {
	loop = malloc(sizeof(uv_loop_t));
	int r; // uv�⺯����������ֵ

	// ��ʼ���¼�ѭ��
	uv_loop_init(loop);
	uv_tcp_init(loop, &server);

	// ��ʼ������˵�ַ
	struct sockaddr_in address;
	uv_ip4_addr("0.0.0.0", 8000, &address);

	// �󶨵�ַ
	r = uv_tcp_bind(&server, (struct sockaddr*)&address, 0);
	HANDLE_UV_ERR_IF_NEED(r, "uv_tcp_bind");

	// ��ʼ����
	int backlog = 128; // �ں˿����ڶ����ۻ��Ĵ�С������linux��listen����
	r = uv_listen((struct uv_stream_s*)&server, 128, on_connection);
	HANDLE_UV_ERR_IF_NEED(r, "uv_listen");

	// ��ʼ�¼�ѭ��
	uv_run_mode mode = UV_RUN_DEFAULT;
	uv_run(loop, mode);
}

