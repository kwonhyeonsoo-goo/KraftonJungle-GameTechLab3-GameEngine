struct AppContext;
class RenderQueue;

class RenderSceneExtractor {
public:
    // World 오브젝트 → RenderCommand 변환 후 큐에 추가
    // 포인터 소비는 여기서 끝, 큐에는 값만 들어간다
    static void Extract(const AppContext& ctx, RenderQueue& queue);
};