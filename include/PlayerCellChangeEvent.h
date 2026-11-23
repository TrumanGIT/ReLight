#pragma once

struct CellChangeEvent : RE::BSTEventSink<RE::BGSActorCellEvent> {

    static void registerEventSink();


    static CellChangeEvent* GetSingleton()
    {
        static CellChangeEvent singleton;
        return &singleton;
    }


private:
    RE::BSEventNotifyControl ProcessEvent(const RE::BGSActorCellEvent* a_event, RE::BSTEventSource<RE::BGSActorCellEvent>*) override;
};