namespace Movement
{
    using namespace Tasks;

    class UnitMovementImpl;

    class MoveSplineUpdatable
    {
    private:
        MSTime m_lastQuery;
        MoveSpline m_base;
        UnitMovementImpl& m_owner;
        IListener * m_listener;
        bool m_moving;
        TaskTarget_DEV m_task;

        std::vector<OnEventArgs> events;

    private:

        enum{
        /** Spline movement update frequency, milliseconds */
            UpdateDelay = 400,
        };

        inline MSTime NextUpdateTime() const {
            return m_lastQuery + std::min(m_base.next_timestamp() - m_base.timePassed(), (int32)UpdateDelay);
        }

        void recache(int32 recacheDelay = 100);

        void PrepareMoveSplineArgs(MoveSplineInitArgs& args, UnitMoveFlag& moveFlag_new);

        void Execute(TaskExecutor_Args& args);

        void Disable();

    public:
        bool isEnabled() const { return m_moving;}
        void SetListener(IListener * listener) { m_listener = listener;}
        void ResetLisener() { m_listener = NULL; }

        void Launch(MoveSplineInitArgs& args);
        std::string ToString() const { return m_base.ToString();}

        explicit MoveSplineUpdatable(UnitMovementImpl& owner);

        ~MoveSplineUpdatable() {
            Disable();
            m_task.Unregister();
            ResetLisener();
        }

        const MoveSpline& moveSpline() const { return m_base;}

        uint32 getCurrentMoveId() const
        {
            if (isEnabled())
                return m_base.GetId();
            else
                return 0;
        }

        uint32 getLastMoveId() const {
            return m_base.GetId();
        }

        MSTime ArriveTime() const {
            return m_lastQuery + m_base.timeElapsed();
        }
    };

    class MoveSplineUpdatablePtr
    {
        MoveSplineUpdatable m_base;
    public:
        explicit MoveSplineUpdatablePtr(UnitMovementImpl& owner) : m_base(owner) {}

        ~MoveSplineUpdatablePtr() {
        }

        inline MoveSplineUpdatable* operator ->() {
            return &m_base;
        }
        inline const MoveSplineUpdatable* operator ->() const {
            return &m_base;
        }
    };
}
