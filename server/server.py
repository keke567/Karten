import asyncio
from datetime import datetime

def logger(msg : str, type = "log"):
    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    print(timestamp + f" [{type}] {msg}")

class server:
    _clients = set()
    
    async def broadcast(self, message : str, sender = None):
        for client in self._clients:
            if client != sender:
                client.write(message.encode())
                await client.drain() # ??? client.drain?
                
    async def handle_client(self, reader : asyncio.StreamReader, writer : asyncio.StreamWriter):
        addr = writer.get_extra_info("peername") # ??? writer? writer.get_rxtra_info?
        
        self._clients.add(writer)
        
        try:
            writer.write(f"welcome:{len(self._clients)}\r\n".encode("utf-8"))
            logger(f'user "{addr}" has joined.')
            await writer.drain()
            
            await self.broadcast(f"user:{addr} has joined\r\n", writer)
            
            while True:
                data = await reader.readline()
                if not data:break
                
                msg = data.decode("utf-8").strip()
                if msg:
                    logger(f"{addr}:{msg}")
                    await self.broadcast(f"{addr}:{msg}\r\n", writer)
                    
        except Exception as e:
            logger(f"Connection error: {e}", type = "error")
        finally:
            # 清理连接
            self._clients.remove(writer)
            writer.close()
            await writer.wait_closed()
            logger(f'user "{addr}" exits.')
            await self.broadcast(f"用户 {addr} 离开了聊天室\r\n")
            
    async def main(self):
        """
        服务器主程序
        """
        server = await asyncio.start_server(
            self.handle_client,
            '0.0.0.0',  # 监听所有网络接口
            8888
        )
        
        # 获取服务器地址
        addrs = ', '.join(str(sock.getsockname()) for sock in server.sockets)
        logger("Server starts.")
        
        # 运行服务器
        async with server:
            await server.serve_forever()

a = server()
# 运行服务器
try:
    asyncio.run(a.main())
except KeyboardInterrupt:
    logger("Server stops.")