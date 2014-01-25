#ifndef AUTOHOSTMANAGER_H
#define AUTOHOSTMANAGER_H

#include <wx/string.h>
#include <exception>
class Battle;


class AutohostHandler
{
    public:
        AutohostHandler();
        virtual ~AutohostHandler();

        virtual void Balance() const {};
        virtual void SetRandomMap() const {};
        virtual void SetMap(const wxString& /*map*/) const {};
        virtual void ClearStartBoxes() const {};
        virtual void AddStartBox(int /*posx*/,int /*posy*/,int /*w*/,int /*h*/) const {};
        virtual void Notify() const {};
        virtual void Start() const {};
    protected:
        Battle* m_battle;

    private:
        void SetBattle(Battle* battle);


        friend class AutohostManager;
};

class SpringieHandler: public AutohostHandler
{
    public:
        SpringieHandler();
        ~SpringieHandler();

        void Balance() const ;
        void SetRandomMap() const ;
        void SetMap(const wxString& map) const ;
        void ClearStartBoxes() const ;
        void AddStartBox(int posx,int posy,int w,int h) const ;
        void Notify() const ;
		void Start() const ;
};

class SpadsHandler: virtual public AutohostHandler
{
    public:
        SpadsHandler();
        ~SpadsHandler();

        void Balance() const ;
        void SetRandomMap() const ;
        void SetMap(const wxString& map) const ;
        void ClearStartBoxes() const ;
        void AddStartBox(int posx,int posy,int w,int h) const ;
        void Notify() const ;
        void Start() const ;
};

class AutohostManager
{
    public:

        enum AutohostType
        {
        	AUTOHOSTTYPE_NONE,
            AUTOHOSTTYPE_UNKNOWN,
            AUTOHOSTTYPE_SPRINGIE,
            AUTOHOSTTYPE_SPADS
        };

        AutohostManager();
        ~AutohostManager();

        void SetBattle(Battle* bt);

        bool RecnognizeAutohost();
		bool RecnognizeAutohost(const wxString& game_hosttype_val);
        bool RecnognizeAutohost(const wxString& who, const wxString& message);

        AutohostType GetAutohostType() const ;

        const AutohostHandler& GetAutohostHandler() const ;

        const SpringieHandler& GetSpringie() const ;
        const SpadsHandler& GetSpads() const ;
    private:
        SpringieHandler m_springie;
        SpadsHandler m_spads;
        AutohostHandler m_emptyhandler;

        AutohostType m_type;
        Battle* m_battle;
};

#endif // AUTOHOSTMANAGER_H
