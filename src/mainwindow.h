#pragma once

#include "backend.h"
#include "lib/copy_move.h"

#include <QFlags>
#include <QMainWindow>

#include <memory>
#include <optional>

// # GUI state mutation tracking (StateTransaction):

/// Used to find which portion of the GUI needs to be redrawn.
enum class StateUpdateFlag : uint32_t {
    None = 0,
    All = ~(uint32_t)0,
    ChipsEdited = 0x1,
    FileReplaced = 0x10,
    SettingsChanged = 0x20,
};
Q_DECLARE_FLAGS(StateUpdateFlags, StateUpdateFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(StateUpdateFlags)

class MainWindowImpl;
class Backend;

class [[nodiscard]] StateTransaction {
private:
    MainWindowImpl * _win;

    int _uncaught_exceptions;

    /// Which part of the GUI needs to be redrawn due to events.
    StateUpdateFlags _queued_updates = StateUpdateFlag::None;

// impl
private:
    /// Do not call directly; use MainWindow::edit_state() or edit_unwrap() instead.
    StateTransaction(MainWindowImpl * win);

public:
    static std::optional<StateTransaction> make(MainWindowImpl * win);

    DISABLE_COPY(StateTransaction)
    StateTransaction & operator=(StateTransaction &&) = delete;
    StateTransaction(StateTransaction && other) noexcept;

    /// Uses the destructor to update the GUI in response to changes,
    /// so does nontrivial work and could throw exceptions
    /// (no clue if exceptions propagate through Qt).
    ~StateTransaction() noexcept(false);

    Backend const& state() const;
private:
    Backend & state_mut();

// Mutations:
public:
    void update_all() {
        _queued_updates = StateUpdateFlag::All;
    }

    void file_replaced();
    void chips_changed();
    void settings_changed();
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    Backend _backend;

protected:
    // MainWindow()
    using QMainWindow::QMainWindow;

public:
    /// If path is empty, ignored.
    static std::unique_ptr<MainWindow> new_with_path(QString path);

    /// Fails if another edit transaction is logging or responding to changes.
    ///
    /// QActions triggered by the user should never occur reentrantly,
    /// so use `unwrap(edit_state())` or `edit_unwrap()` (throws exception if concurrent).
    ///
    /// Signals generated by widgets (eg. spinboxes) may occur reentrantly
    /// if you forget to use QSignalBlocker when ~StateTransaction() sets widget values,
    /// so use `debug_unwrap(edit_state(), lambda)` (if edit in progress,
    /// assert on debug builds and otherwise skip function call)
    virtual std::optional<StateTransaction> edit_state() = 0;

    virtual StateTransaction edit_unwrap() = 0;
};
