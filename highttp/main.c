#include <stdio.h>
#include <io.h>
#include <uv.h>

#define HANDLE_UV_ERR_IF_NEED(r, func_name) \
	if (r < 0) { \
		fprintf(stderr, "%s: %s\n", func_name, uv_strerror(r)); \
		getchar(); \
		exit(1); \
	}

static uv_loop_t *loop; // 创建事件循环
static uv_tcp_t server;  // tcp服务端handle

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
	// 释放时报错，暂时注释掉。未知道uv是否需要调用者释放client空间
	//free(client);
}

void read_cb(struct uv_stream_s *client, ssize_t nread, const struct uv_buf_t* buf) {
	if (buf->base == NULL) return;
	if (nread >= 0) {
		// TODO: 解析HTPP请求报文
		_write(1, buf->base, nread);
	} else {
		if (nread == UV_EOF) {
			uv_close((struct uv_handle_s*)client, on_close);
		}
	}
	free(buf->base);
}

/**
 * \brief uv_listen的回调函数。当有新的连接建立时会被调用
 * \param server_handle tcp服务端handle
 * \param status 未知
 */
void on_connection(struct uv_stream_s *server_handle, int status) {
	int r;
	printf("connected\n");
	struct uv_tcp_t *client = malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, client);

	// 接受请求
	r = uv_accept(server_handle, (uv_stream_t*)client);
	HANDLE_UV_ERR_IF_NEED(r, "uv_accept");

	// 读取数据
	r = uv_read_start((uv_stream_t*)client, alloc_cb, read_cb);
	HANDLE_UV_ERR_IF_NEED(r, "uv_read_start");
}

int main(int argc, char* argv[]) {
	loop = malloc(sizeof(uv_loop_t));
	int r; // uv库函数操作返回值

	// 初始化事件循环
	uv_loop_init(loop);
	uv_tcp_init(loop, &server);

	// 初始化服务端地址
	struct sockaddr_in address;
	uv_ip4_addr("0.0.0.0", 8000, &address);

	// 绑定地址
	r = uv_tcp_bind(&server, (struct sockaddr*)&address, 0);
	HANDLE_UV_ERR_IF_NEED(r, "uv_tcp_bind");

	// 开始监听
	int backlog = 128; // 内核可以在队列累积的大小，就如linux的listen调用
	r = uv_listen((struct uv_stream_s*)&server, 128, on_connection);
	HANDLE_UV_ERR_IF_NEED(r, "uv_listen");

	// 开始事件循环
	uv_run_mode mode = UV_RUN_DEFAULT;
	uv_run(loop, mode);
}

