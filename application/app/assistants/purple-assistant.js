
PurpleAssistant = function (params) {
    this.template = params.initialTemplate || params.template;

    this.username = params.username || "";
    this.password = params.password || "";
    this.prpl = this.template.prpl;
    this.prefs = {};
    this.options = {};
    this.optionsModels = {};
};

var _ValidatorAddress = "palm://org.webosinternals.purple.validator/";

PurpleAssistant.prototype.setup = function() {
    this.controller.get('header').innerHTML = this.template.loc_name;

    // Username
    this.usernameModel = {
        value: this.username,
        disabled: false
    };

    this.controller.get('loc_usernameLabel').innerHTML =
        this.template.loc_usernameLabel || "Username";

    this.controller.setupWidget('username',
        {
            hintText: this.template.loc_usernameExplainLabel || "",
            focus: true
        },
        this.usernameModel
    );

    // Password
    this.passwordModel = {
        value: this.password,
        disabled: false
    };

    this.controller.get('loc_passwordLabel').innerHTML =
        this.template.loc_passwordLabel || "Password";

    this.controller.setupWidget('password',
        {
            textFieldName: "loc_passwordLabel",
            hintText: this.template.loc_passwordExplainLabel || ""
        },
        this.passwordModel
    );

    // Setup CreateAccount widget
    this.controller.setupWidget('CreateAccountButton',
        {
            type: Mojo.Widget.activityButton
        },
        {
            buttonClass: 'primary',
            buttonLabel: 'Sign In',
            disabled: false
        }
    );

    // Setup OptionsList
    this.optionsModel = {
        label: "Advanced Options!",
        items: []
    };

    this.controller.setupWidget("OptionsList", {
            itemTemplate: "templates/options-item",
            swipeToDelete: false,
            autoconfirmDelete: false,
            reorderable: false
        },
        this.optionsModel);

    // Call getOptions
    this.controller.serviceRequest(_ValidatorAddress + "getOptions", {
        parameters: {
            prpl: this.template.prpl,
            locale: Mojo.Locale.getCurrentLocale()
        },
        onSuccess: this.optionsSuccess.bind(this)
        // TODO: onFailure: Return
    });
};

PurpleAssistant.prototype.optionsSuccess = function(response) {
    Mojo.Log.error("optionsSuccess: " + JSON.stringify(response));
    this.createOptionsWidget(response.options);

    this.controller.listen(this.controller.get('CreateAccountButton'),
            Mojo.Event.tap,
            this.createAccount.bindAsEventListener(this)
        );
};

PurpleAssistant.prototype.createAccount = function(ev) {
    if (this.usernameModel.value == "")
    {
        this.showError("Validation", "Please enter a valid username");
        return;
    }

    this.disableControls();

    this.controller.serviceRequest(_ValidatorAddress + "validateAccount", {
        parameters: {
            "username": this.usernameModel.value,
            "password": this.passwordModel.value,
            "prpl": this.template.prpl,
            "templateId": this.template.templateId,
            "config": this.prefs
        },
        onFailure: this.eventFail.bind(this),
        onError: this.eventFail.bind(this)
    });

    this.getEvent();
};

PurpleAssistant.prototype.eventSuccess = function(response) {
    if ("credentials" in response)
    {
        var result = {
            template: this.template,
            username: response.username || this.usernameModel.value,
            config: this.prefs,
            defaultResult: {
                result: {
                    returnValue: true,
                    credentials: response.credentials
                }
            }
        };

        this.controller.stageController.popScene(result);
    }
    else
    {
        this.showPopup(response);
    }
};

PurpleAssistant.prototype.eventFail = function(response) {
    Mojo.Log.error(JSON.stringify(response))
    if (response.errorCode) {
        // TODO: Show error message
    }
    else
    {
        // TODO: Hide error message
    }

    this.enableControls();
};

PurpleAssistant.prototype.enableControls = function(val) {
    this.controller.get('CreateAccountButton').mojo.deactivate();
    this.toggleControls(true);
};

PurpleAssistant.prototype.disableControls = function(val) {
    this.toggleControls(false);
};

PurpleAssistant.prototype.toggleControls = function(val) {
    val = val && true || false;
    this.usernameModel.disabled = val;
    this.passwordModel.disabled = val;

    this.controller.modelChanged(this.usernameModel);
    this.controller.modelChanged(this.passwordModel);

    for (var name in this.optionsModels)
    {
        this.optionsModels[name].disabled = val;
        this.controller.modelChanged(this.optionsModels[name]);
    }
};

PurpleAssistant.prototype.showPopup = function(popup) {
    var dialog = {
        message: ""
    };
    if ("title" in popup)
        dialog.title = popup.title;
    if ("primary" in response)
        dialog.message += popup.primary;
    if ("secondary" in response)
    {
        if (dialog.message != "")
            dialog.message += "<br/>";
        dialog.message += popup.secondary;
        dialog.allowHTMLMessage = true;
    }

    if ("actions" in popup)
    {
        var choices = [];
        for (var i in popup.actions)
        {
            choices.push({
                label: popup.actions[i],
                value: {
                    id: popup.actions[i].id,
                    request_id: popup.actions[i].request_id
                }
            });
        }

        dialog.choices = choices;
        dialog.preventCancel = true;

        dialog.onChoose = function(value) {
            new Mojo.Service.Request(_ValidatorAddress + "answerEvent", {
                parameters: {
                    answer: value.id,
                    id: value.request_id
                },
                onResponse: function() {
                    this.getEvent();
                }
            });
        };

    }
    else if ("type" in popup)
    {
    }

    this.controller.showAlertDialog(dialog);
};

PurpleAssistant.prototype.getEvent = function() {
    this.controller.serviceRequest(_ValidatorAddress + "getEvent", {
        parameters: {},
        onSuccess: this.eventSuccess.bind(this),
        onFailure: this.eventFail.bind(this),
        onError: this.eventFail.bind(this)
    });
};

var _TypeToMojoElement = {
    "string": "TextField",
    "int": "TextField",
    "bool": "CheckBox",
    "list": "ListSelector"
};

PurpleAssistant.prototype.createOptionsWidget = function(options) {
    for (var name in options) {
        var node = options[name];
        if (node.type in _TypeToMojoElement)
        {
            this.optionsModel.items.push({
                name: name,
                text: node.text,
                mojo_element: _TypeToMojoElement[node.type]
            });
        }
    }

    this.controller.modelChanged(this.optionsModel);
    this.options = options;

    for (var name in options) {
        var node = options[name];

        var attributes = {};
        var model = { disabled: false };

        if ("default_value" in node)
            model.value = node.default_value;

        if (node.type == "string")
        {
            attributes.changeOnKeyPress = true;
        }
        else if (node.type == "int")
        {
            attributes.changeOnKeyPress = true;
            attributes.charsAllow = function(c) {
                if (/[0-9]/.test(c))
                    return true;
                else
                    return false;
            }
        }
        else if (node.type == "bool")
        {
        }
        else if (node.type == "list")
        {
            model.choices = [];
            for (var name_ in node.choices)
                model.choices.push({
                    label: node.choices[name_],
                    value: name_
                });
            if ("default_value" in node)
                model.value = node.default_value;
        }

        this.optionsModels[name] = model;

        this.controller.setupWidget(name, attributes, this.optionsModels[name]);
        Mojo.Log.info("Setup widget", name, attributes);
        this.controller.listen(
                this.controller.get(name),
                Mojo.Event.propertyChange,
                function (ev) {
                    this.prefs[name] = ev.value;
                }
        );
    }
};

