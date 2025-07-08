// websocket.js
let socket = null;
let currentRoomId = null;
let currentPlayerIdx = null;

// 初始化WebSocket连接
function connectWebSocket() {
    const wsProtocol = window.location.protocol === 'https:' ? 'wss://' : 'ws://';
    const wsUrl = wsProtocol + window.location.host + '/ws';

    socket = new WebSocket(wsUrl, ['game-protocol']);

    socket.onopen = function (e) {
        console.log('WebSocket连接已建立');
    };

    socket.onmessage = function (event) {
        const response = JSON.parse(event.data);
        handleServerMessage(response);
    };

    socket.onclose = function (event) {
        if (event.wasClean) {
            console.log(`连接关闭，代码=${event.code} 原因=${event.reason}`);
        } else {
            console.log('连接中断');
            // 尝试重新连接
            setTimeout(connectWebSocket, 3000);
        }
    };

    socket.onerror = function (error) {
        console.error('WebSocket错误:', error);
    };
}

// 处理服务器消息
function handleServerMessage(response) {
    console.log('收到服务器消息:', response);

    switch (response.action) {
        case 'create':
            if (response.success) {
                currentRoomId = response.room_id;
                currentPlayerIdx = response.player_idx;
                alert(`房间创建成功！房间号: ${currentRoomId}`);
                // 跳转到游戏等待页面
                window.location.href = `/waiting-room?room_id=${currentRoomId}&player_idx=${currentPlayerIdx}`;
            } else {
                alert('创建房间失败: ' + response.message);
            }
            break;

        case 'join':
            if (response.success) {
                currentRoomId = response.room_id;
                currentPlayerIdx = response.player_idx;
                alert(`成功加入房间！房间号: ${currentRoomId}，您是玩家 ${currentPlayerIdx + 1}`);
                // 跳转到游戏等待页面
                window.location.href = `/waiting-room?room_id=${currentRoomId}&player_idx=${currentPlayerIdx}`;
            } else {
                alert('加入房间失败: ' + response.message);
            }
            break;

        default:
            console.log('未知的action类型:', response.action);
    }
}

// 发送创建房间请求
function sendCreateRoomRequest(playerNum, password) {
    if (!socket || socket.readyState !== WebSocket.OPEN) {
        alert('连接尚未建立，请稍后再试');
        return false;
    }

    const request = {
        action: "create",
        room_info: {
            player_num: playerNum,
            passwd: password || ""
        }
    };

    socket.send(JSON.stringify(request));
    return true;
}

// 发送加入房间请求
function sendJoinRoomRequest(roomId, password) {
    if (!socket || socket.readyState !== WebSocket.OPEN) {
        alert('连接尚未建立，请稍后再试');
        return false;
    }

    const request = {
        action: "join",
        room_id: roomId,
        passwd: password || ""
    };

    socket.send(JSON.stringify(request));
    return true;
}

// 页面加载时建立连接
window.addEventListener('load', function () {
    connectWebSocket();
});