def create_piece(board, pos=[0, 0]):
    current_pos = pos
    n = len(board)
    def move(orien, step):
        nonlocal current_pos
        if step == 0:
            return current_pos
        for i in range(1, step + 1):
            new_pos = [current_pos[0] + i * orien[0], current_pos[1] + i * orien[1]]
            if new_pos[0] < 0 or new_pos[0] >= n or new_pos[1] < 0 or new_pos[1] >= n or board[new_pos[0]][new_pos[1]] == 'x':
                return 'Wrong move'
        current_pos = [current_pos[0] + step * orien[0], current_pos[1] + step * orien[1]]
        return current_pos
    return move
n, start_x, start_y = map(int, input().split())

# 读取棋盘
board = []
for _ in range(n):
    row = input().strip().split()
    board.append(row)
    
# 创建棋子
piece = create_piece(board, [start_x, start_y])

# 处理移动指令
try:
    while True:
        cmd = input().strip()
        dx, dy, step = map(int, cmd.split())
        result = piece([dx, dy], step)
        print(result)
except EOFError:
    pass  # 读取到EOF时正常退出
