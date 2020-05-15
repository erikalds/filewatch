import React from 'react';
import ReactDOM from 'react-dom';
import TreeMenu from 'react-simple-tree-menu';
import './index.css';

class Filelist extends React.Component {
    render() {
        const treeData = {
            'first-level-node-1': {               // key
                label: 'Node 1 at the first level',
                index: 0, // decide the rendering order on the same level
                //...,      // any other props you need, e.g. url
                nodes: {
                    'second-level-node-1': {
                        label: 'Node 1 at the second level',
                        index: 0,
                        nodes: {
                            'third-level-node-1': {
                                label: 'Node 1 at the third level',
                                index: 0,
                                nodes: {} // you can remove the nodes property or leave it as an empty array
                            },
                        },
                    },
                },
            },
            'first-level-node-2': {
                label: 'Node 2 at the first level',
                index: 1,
            },
        };
        return (
            <div className="Filelist">
                <TreeMenu data={treeData} />
            </div>
        );
    }
}

class FilewatcherView extends React.Component {
    render() {
        return (
            <div className="FilewatcherView">
                <h1>Filewatcher</h1>
                <Filelist/>
            </div>
        );
    }
}

ReactDOM.render(
        <FilewatcherView />,
        document.getElementById('root')
);
