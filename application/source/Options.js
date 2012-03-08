function getOptions() {
    return {
            "bla" : { type: "int", text: "Integer parameter", default_value:1 },
            "blubb": { type: "bool", text: "Bool parameter", default_value: false },
            "brabbel": { type: "list", text: "List parameter",
                         choices: {
                            "Pizza": "pizza",
                            "Pasta": "pasta"
                         }
                    }
        }
}

enyo.kind({
    name: "Messaging.Options",
    kind: "RowGroup",
    caption: "Advanced Options",
    events: {
        onPreferenceChanged: ""
    },
    options: {},

    create: function() {
        this.inherited(arguments);

        // TODO: call getOptions, store in options
        
        options = getOptions();

        this._makeComponents(options);
        this.render();
    },

    _makeComponents: function(options) {
        console.log(options);
        for (var name in options)
        {
            var node = options[name];

            var innerComponent = new Object();

            if (node.type == "string")
            {
                innerComponent = {
                    kind: "Input",
                    hint: node.type,
                    autocorrect: false,
                    spellcheck: false,
                    value: node.default_value,
                    onChange: "onPreferenceChanged"
                }
            }
            else if (node.type == "int")
            {
                innerComponent = {
                    kind: "Input",
                    hint: node.type,
                    autocorrect: false,
                    spellcheck: false,
                    value: node.default_value,
                    onChange: function(inSender) {
                        alert("call");
                        this.options[name] = int(inSender.getValue());
                        inSender.setValue(this.options[name])
                    }
                };
            }
            else if (node.type == "bool")
            {
                innerComponent = {
                    kind: "CheckBox",
                    value: node.default_value,
                    onChange: function(inSender) {
                        this.options[name] = inSender.getChecked();
                    }
                };
            }
            else if (node.type == "list")
            {
                items = [];
                for (var name_ in node.choices)
                    items.push(node.choices[name_]);
                /*
                    items.push({caption: node.choices[name_],
                                value: name_
                                });*/

                innerComponent = {
                    kind: "PopupSelect",
                    items: ["asdf", "bla"],
                    onSelect: function(inSender) {
                        this.options[name] = inSender.items[inSender.selected].value;
                    }
                };
            }
            else
            {
                // TODO: ERROR!
            }

            innerComponent["name"] = name;
            console.log(innerComponent);

            this.createComponent({
                layoutKind: "HFlexLayout",
                components: [
                    {content: node.text, flex: 1},
                    innerComponent
                ]
            });
        }
    },
})
