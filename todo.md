# TODO:

* multi-threaded queue job system
* cross platform mutex implementation (and threads)
* platform independent threading
  (pthreads on unix, winapi on windows)
* crosss platform file watching (inotify on linux, FindFirstChangeNotification on windows, fsevents on mac)
* drop support for vcpkg in turn for git submodules and custom build scripts
* implement nanovg as the backend for the gui

`types`

```

style RedButton {
    color: red
    border: 1px solid red
    padding: 5px
    margin: 5px
}

component Button {
    text: string
    id: string
    onClick: string
    style: RedButton
}

Window (title: "My Application", width: 800, height=600) {
    VBox {
        Button (text: "Click Me", id: "clickButton, onClick: "buttonClicked")
        Label (text: "Hello, World!", id: "helloLabel")
        HBox {
            CheckBox (text: "Check me", id: "check1" onCheck: "checkboxChecked")
            {{ if is_checked then 
                Label (text: "Checked", id: "checkedLabel")
            else 
                Label (text: "Not Checked", id: "notCheckedLabel")
            end}} 
        }
    }
}
```